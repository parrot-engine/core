#include <ctype.h>
#include <stdio.h>

#include "parrot/core/array.h"
#include "parrot/core/regex.h"
#include "parrot/core/scope.h"

// Heavily modified from https://github.com/kokke/tiny-regex-c

#define MAX_CLASS_SIZE (64)

typedef enum {
    ParrotRegexTokenType_EOF,
    ParrotRegexTokenType_DOT,
    ParrotRegexTokenType_BEGIN,
    ParrotRegexTokenType_END,
    ParrotRegexTokenType_QUESTIONMARK,
    ParrotRegexTokenType_STAR,
    ParrotRegexTokenType_PLUS,
    ParrotRegexTokenType_CHAR,
    ParrotRegexTokenType_CHAR_CLASS,
    ParrotRegexTokenType_INV_CHAR_CLASS,
    ParrotRegexTokenType_DIGIT,
    ParrotRegexTokenType_NOT_DIGIT,
    ParrotRegexTokenType_ALPHA,
    ParrotRegexTokenType_NOT_ALPHA,
    ParrotRegexTokenType_WHITESPACE,
    ParrotRegexTokenType_NOT_WHITESPACE,
    ParrotRegexTokenType_GROUP,
} ParrotRegexTokenType;

typedef struct ParrotRegexToken ParrotRegexToken;
struct ParrotRegexToken {
    ParrotRegexTokenType type;
    union {
        char char_data;
        char *class_data;
        struct {
            ParrotRegexToken **p_arr_tokens;
        } group_data;
    } data;
};

struct ParrotRegex {
    ParrotScope *scope;

    ParrotRegexToken *arr_expr;
    char **arr_class_buf;
};

static const char *next_token(ParrotScope *scope, const char *pattern, ParrotRegexToken *out) {
    char c = *pattern++;
    memset(out, 0, sizeof(ParrotRegexToken));

    switch (c) {
    case '^': {
        out->type = ParrotRegexTokenType_BEGIN;
    } break;
    case '$': {
        out->type = ParrotRegexTokenType_END;
    } break;
    case '.': {
        out->type = ParrotRegexTokenType_DOT;
    } break;
    case '*': {
        out->type = ParrotRegexTokenType_STAR;
    } break;
    case '+': {
        out->type = ParrotRegexTokenType_PLUS;
    } break;
    case '?': {
        out->type = ParrotRegexTokenType_QUESTIONMARK;
    } break;
    case '\\': {
        switch (*pattern) {
        case '\0':
            break;

        case 'd': {
            out->type = ParrotRegexTokenType_DIGIT;
        } break;
        case 'D': {
            out->type = ParrotRegexTokenType_NOT_DIGIT;
        } break;
        case 'w': {
            out->type = ParrotRegexTokenType_ALPHA;
        } break;
        case 'W': {
            out->type = ParrotRegexTokenType_NOT_ALPHA;
        } break;
        case 's': {
            out->type = ParrotRegexTokenType_WHITESPACE;
        } break;
        case 'S': {
            out->type = ParrotRegexTokenType_NOT_WHITESPACE;
        } break;

        default: {
            out->type = ParrotRegexTokenType_CHAR;
            out->data.char_data = *pattern;
        } break;
        }

        // Skip the character
        pattern++;
    } break;
    case '[': {
        size_t class_index = 0;
        char *class = calloc(MAX_CLASS_SIZE + 1, sizeof(char));
        ParrotScope_push_free(scope, class);

        out->type = ParrotRegexTokenType_CHAR_CLASS;

        if (*pattern == '^') {
            pattern++;
            out->type = ParrotRegexTokenType_INV_CHAR_CLASS;

            if (!(*pattern)) {
                return NULL;
            }
        }

        while (class_index <= MAX_CLASS_SIZE && *pattern != ']') {
            if (!(*pattern)) {
                // ']' was not found before end of string
                return NULL;
            }

            if (*pattern == '\\') {
                pattern++;
                if (!(*pattern)) {
                    return NULL;
                }

                class[class_index++] = *pattern++;
                continue;
            }

            class[class_index++] = *pattern++;
        }

        // Skip ']'
        pattern++;

        out->data.class_data = class;
    } break;
    case '(': {
        out->type = ParrotRegexTokenType_GROUP;

        out->data.group_data.p_arr_tokens = malloc(sizeof(*out->data.group_data.p_arr_tokens));
        ParrotScope_push_free(scope, out->data.group_data.p_arr_tokens);
        *out->data.group_data.p_arr_tokens = NULL;
        ParrotScope_push_arrfree(scope, *out->data.group_data.p_arr_tokens);

        ParrotRegexToken sub_token = {0};
        while (*pattern != ')') {
            if (!(*pattern)) {
                return NULL;
            }

            pattern = next_token(scope, pattern, &sub_token);
            if (!pattern) {
                return NULL;
            }

            ParrotArray_push(*out->data.group_data.p_arr_tokens, sub_token);
        }

        // Skip ')'
        pattern++;

        ParrotArray_push(*out->data.group_data.p_arr_tokens,
                         ((ParrotRegexToken){
                             .type = ParrotRegexTokenType_EOF,
                         }));
    } break;
    default: {
        out->type = ParrotRegexTokenType_CHAR;
        out->data.char_data = c;
    } break;
    }

    if (!c) {
        return NULL;
    }

    return pattern;
}

ParrotRegex *ParrotRegex_new_or_null(const char *pattern) {
    ParrotRegex *self = PARROT_ALLOC(ParrotRegex);

    self->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(self->scope, self);

    ParrotScope_push_arrfree(self->scope, self->arr_expr);
    ParrotScope_push_arrfree(self->scope, self->arr_class_buf);

    while (*pattern) {
        ParrotRegexToken token = {0};
        pattern = next_token(self->scope, pattern, &token);

        if (!pattern) {
            ParrotScope_delete(self->scope);
            return NULL;
        }

        ParrotArray_push(self->arr_expr, token);
    }

    ParrotArray_push(self->arr_expr,
                     ((ParrotRegexToken){
                         .type = ParrotRegexTokenType_EOF,
                     }));

    return self;
}

void ParrotRegex_delete(ParrotRegex *self) {
    PARROT_FAIL_NULL(self);

    ParrotScope_delete(self->scope);
}

void ParrotRegex_vdelete(void *self) {
    ParrotScope_delete(self);
}

static bool match_alphanum(char c) {
    return ((c == '_') || isalpha(c) || isdigit(c));
}

static bool match_range(char c, const char *str) {
    return ((c != '-') && (str[0] != '\0') && (str[0] != '-') && (str[1] == '-') && (str[2] != '\0') &&
            ((c >= str[0]) && (c <= str[2])));
}

static bool match_dot(char c) {
    (void)c;
    return true;
}

static bool match_metachar(char c, const char *str) {
    switch (str[0]) {
    case 'd':
        return isdigit(c);
    case 'D':
        return !isdigit(c);
    case 'w':
        return match_alphanum(c);
    case 'W':
        return !match_alphanum(c);
    case 's':
        return isspace(c);
    case 'S':
        return !isspace(c);
    default:
        return (c == str[0]);
    }
}

static bool is_metachar(char c) {
    return ((c == 's') || (c == 'S') || (c == 'w') || (c == 'W') || (c == 'd') || (c == 'D'));
}

static bool match_charclass(char c, const char *str) {
    do {
        if (match_range(c, str)) {
            return true;
        } else if (str[0] == '\\') {
            str++;
            if (match_metachar(c, str)) {
                return true;
            } else if ((c == str[0]) && !is_metachar(c)) {
                return true;
            }
        } else if (c == str[0]) {
            if (c == '-') {
                return ((str[-1] == '\0') || (str[1] == '\0'));
            } else {
                return true;
            }
        }
    } while (*str++ != '\0');

    return false;
}

static bool match_one(ParrotRegexToken p, char c) {
    switch (p.type) {
    case ParrotRegexTokenType_DOT:
        return match_dot(c);
    case ParrotRegexTokenType_CHAR_CLASS:
        return match_charclass(c, (const char *)p.data.class_data);
    case ParrotRegexTokenType_INV_CHAR_CLASS:
        return !match_charclass(c, (const char *)p.data.class_data);
    case ParrotRegexTokenType_DIGIT:
        return isdigit(c);
    case ParrotRegexTokenType_NOT_DIGIT:
        return !isdigit(c);
    case ParrotRegexTokenType_ALPHA:
        return isalnum(c);
    case ParrotRegexTokenType_NOT_ALPHA:
        return !isalnum(c);
    case ParrotRegexTokenType_WHITESPACE:
        return isspace(c);
    case ParrotRegexTokenType_NOT_WHITESPACE:
        return !isspace(c);
    default:
        return (p.data.char_data == c);
    }
}

static bool match_pattern(ParrotRegexToken *pattern, const char *text, size_t *out_size);

static ptrdiff_t match_atom(ParrotRegexToken token, const char *text) {
    if (token.type == ParrotRegexTokenType_GROUP) {
        size_t consumed = 0;
        if (match_pattern(*token.data.group_data.p_arr_tokens, text, &consumed)) {
            return (ptrdiff_t)consumed;
        }
        return -1;
    }

    if (text[0] == '\0') {
        return -1;
    }
    return match_one(token, text[0]) ? 1 : -1;
}

static bool match_star(ParrotRegexToken p, ParrotRegexToken *pattern, const char *text, size_t *out_size) {
    size_t prelen = *out_size;

    const char *positions[256];
    size_t sizes[256];
    int count = 0;

    positions[count] = text;
    sizes[count++] = *out_size;

    ptrdiff_t step;
    while ((step = match_atom(p, text)) > 0 && count < 256) {
        text += step;
        (*out_size) += step;
        positions[count] = text;
        sizes[count++] = *out_size;
    }

    for (int i = count - 1; i >= 0; i--) {
        *out_size = sizes[i];
        if (match_pattern(pattern, positions[i], out_size))
            return true;
    }

    *out_size = prelen;
    return false;
}

static bool match_plus(ParrotRegexToken token, ParrotRegexToken *pattern, const char *text, size_t *out_size) {
    size_t prelen = *out_size;

    const char *positions[256];
    size_t sizes[256];
    int count = 0;

    ptrdiff_t step;

    while ((step = match_atom(token, text)) > 0 && count < 256) {
        text += step;
        *out_size += step;

        positions[count] = text;
        sizes[count] = *out_size;
        count++;
    }

    for (int i = count - 1; i >= 0; i--) {
        *out_size = sizes[i];

        if (match_pattern(pattern, positions[i], out_size)) {
            return true;
        }
    }

    *out_size = prelen;
    return false;
}

static bool match_question(ParrotRegexToken token, ParrotRegexToken *pattern, const char *text, size_t *out_size) {
    size_t prelen = *out_size;

    if (token.type == ParrotRegexTokenType_EOF) {
        return true;
    }

    if (match_pattern(pattern, text, out_size)) {
        return true;
    }

    *out_size = prelen;

    ptrdiff_t step = match_atom(token, text);

    if (step > 0) {
        if (match_pattern(pattern, text + step, out_size)) {
            *out_size += step;
            return true;
        }
    }

    *out_size = prelen;
    return false;
}

static bool match_pattern(ParrotRegexToken *pattern, const char *text, size_t *out_size) {
    size_t pre = *out_size;
    do {
        if ((pattern[0].type == ParrotRegexTokenType_EOF) || (pattern[1].type == ParrotRegexTokenType_QUESTIONMARK)) {
            return match_question(pattern[0], &pattern[2], text, out_size);
        } else if (pattern[1].type == ParrotRegexTokenType_STAR) {
            return match_star(pattern[0], &pattern[2], text, out_size);
        } else if (pattern[1].type == ParrotRegexTokenType_PLUS) {
            return match_plus(pattern[0], &pattern[2], text, out_size);
        } else if ((pattern[0].type == ParrotRegexTokenType_END) && pattern[1].type == ParrotRegexTokenType_EOF) {
            return (text[0] == '\0');
        }
        (*out_size)++;
    } while ((text[0] != '\0') && match_one(*pattern++, *text++));

    *out_size = pre;
    return false;
}

bool ParrotRegex_match(ParrotRegex *self, const char *text, size_t *out_size) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(text);

    size_t tmp_size = 0;
    if (!out_size) {
        out_size = &tmp_size;
    }

    if (self->arr_expr[0].type == ParrotRegexTokenType_BEGIN) {
        return match_pattern(&self->arr_expr[1], text, out_size);
    }

    return match_pattern(self->arr_expr, text, out_size);
}

ptrdiff_t ParrotRegex_match_position_or_neg(ParrotRegex *self, const char *text, size_t *out_size) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(text);

    for (size_t i = 0; text[i] != '\0'; i++) {
        if (ParrotRegex_match(self, &text[i], out_size)) {
            return i;
        }
    }

    return -1;
}
