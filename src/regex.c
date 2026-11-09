#include <ctype.h>
#include <stdio.h>

#include "parrot/core/array.h"
#include "parrot/core/regex.h"
#include "parrot/core/scope.h"

// Heavily modified from https://github.com/kokke/tiny-regex-c

#define MAX_CLASS_SIZE (64)

typedef enum {
    ParrotRegexExprType_EOF,
    ParrotRegexExprType_DOT,
    ParrotRegexExprType_BEGIN,
    ParrotRegexExprType_END,
    ParrotRegexExprType_QUESTIONMARK,
    ParrotRegexExprType_STAR,
    ParrotRegexExprType_PLUS,
    ParrotRegexExprType_CHAR,
    ParrotRegexExprType_CHAR_CLASS,
    ParrotRegexExprType_INV_CHAR_CLASS,
    ParrotRegexExprType_DIGIT,
    ParrotRegexExprType_NOT_DIGIT,
    ParrotRegexExprType_ALPHA,
    ParrotRegexExprType_NOT_ALPHA,
    ParrotRegexExprType_WHITESPACE,
    ParrotRegexExprType_NOT_WHITESPACE,
} ParrotRegexExprType;

typedef struct {
    unsigned char type;
    union {
        char ch;
        char *class_data;
    } data;
} ParrotRegexExpr;

struct ParrotRegex {
    ParrotScope *scope;

    ParrotRegexExpr *arr_expr;
    char **arr_class_buf;
};

ParrotRegex *ParrotRegex_new(const char *pattern) {
    ParrotRegex *self = PARROT_ALLOC(ParrotRegex);

    self->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(self->scope, self);

    ParrotScope_push_arrfree(self->scope, self->arr_expr);
    ParrotScope_push_arrfree(self->scope, self->arr_class_buf);

    int i = 0;
    while (pattern[i] != '\0') {
        char c = pattern[i];

        ParrotRegexExpr node = {0};

        switch (c) {
        case '^': {
            node.type = ParrotRegexExprType_BEGIN;
        } break;
        case '$': {
            node.type = ParrotRegexExprType_END;
        } break;
        case '.': {
            node.type = ParrotRegexExprType_DOT;
        } break;
        case '*': {
            node.type = ParrotRegexExprType_STAR;
        } break;
        case '+': {
            node.type = ParrotRegexExprType_PLUS;
        } break;
        case '?': {
            node.type = ParrotRegexExprType_QUESTIONMARK;
        } break;

        case '\\': {
            if (pattern[i + 1] != '\0') {
                i++;
                switch (pattern[i]) {
                case 'd': {
                    node.type = ParrotRegexExprType_DIGIT;
                } break;
                case 'D': {
                    node.type = ParrotRegexExprType_NOT_DIGIT;
                } break;
                case 'w': {
                    node.type = ParrotRegexExprType_ALPHA;
                } break;
                case 'W': {
                    node.type = ParrotRegexExprType_NOT_ALPHA;
                } break;
                case 's': {
                    node.type = ParrotRegexExprType_WHITESPACE;
                } break;
                case 'S': {
                    node.type = ParrotRegexExprType_NOT_WHITESPACE;
                } break;

                default: {
                    node.type = ParrotRegexExprType_CHAR;
                    node.data.ch = pattern[i];
                } break;
                }
            }
        } break;

        case '[': {
            size_t class_index = 0;
            char *class = calloc(MAX_CLASS_SIZE + 1, sizeof(char));
            ParrotScope_push_free(self->scope, class);

            node.type = ParrotRegexExprType_CHAR_CLASS;

            if (pattern[i + 1] == '^') {
                node.type = ParrotRegexExprType_INV_CHAR_CLASS;
                i++;

                if (pattern[i + 1] == 0) {
                    ParrotScope_delete(self->scope);
                    return NULL;
                }
            }

            while (class_index <= MAX_CLASS_SIZE && (pattern[++i] != ']') && (pattern[i] != '\0')) {
                if (pattern[i] == '\\') {
                    if (pattern[i + 1] == 0) {
                        ParrotScope_delete(self->scope);
                        return NULL;
                    }

                    class[class_index++] = pattern[i++];
                }

                class[class_index++] = pattern[i];
            }

            class[class_index++] = '\0';
            node.data.class_data = class;
        } break;

        default: {
            node.type = ParrotRegexExprType_CHAR;
            node.data.ch = c;
        } break;
        }

        ParrotArray_push(self->arr_expr, node);

        if (pattern[i] == 0) {
            ParrotScope_delete(self->scope);
            return NULL;
        }

        i++;
    }

    ParrotArray_push(self->arr_expr,
                     ((ParrotRegexExpr){
                         .type = ParrotRegexExprType_EOF,
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

static int matchalphanum(char c) {
    return ((c == '_') || isalpha(c) || isdigit(c));
}

static int matchrange(char c, const char *str) {
    return ((c != '-') && (str[0] != '\0') && (str[0] != '-') && (str[1] == '-') && (str[2] != '\0') &&
            ((c >= str[0]) && (c <= str[2])));
}

static int matchdot(char c) {
#if defined(RE_DOT_MATCHES_NEWLINE) && (RE_DOT_MATCHES_NEWLINE == 1)
    (void)c;
    return 1;
#else
    return c != '\n' && c != '\r';
#endif
}
static int ismetachar(char c) {
    return ((c == 's') || (c == 'S') || (c == 'w') || (c == 'W') || (c == 'd') || (c == 'D'));
}

static int matchmetachar(char c, const char *str) {
    switch (str[0]) {
    case 'd':
        return isdigit(c);
    case 'D':
        return !isdigit(c);
    case 'w':
        return matchalphanum(c);
    case 'W':
        return !matchalphanum(c);
    case 's':
        return isspace(c);
    case 'S':
        return !isspace(c);
    default:
        return (c == str[0]);
    }
}

static bool matchcharclass(char c, const char *str) {
    do {
        if (matchrange(c, str)) {
            return true;
        } else if (str[0] == '\\') {
            str++;
            if (matchmetachar(c, str)) {
                return true;
            } else if ((c == str[0]) && !ismetachar(c)) {
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

static bool matchone(ParrotRegexExpr p, char c) {
    switch (p.type) {
    case ParrotRegexExprType_DOT:
        return matchdot(c);
    case ParrotRegexExprType_CHAR_CLASS:
        return matchcharclass(c, (const char *)p.data.class_data);
    case ParrotRegexExprType_INV_CHAR_CLASS:
        return !matchcharclass(c, (const char *)p.data.class_data);
    case ParrotRegexExprType_DIGIT:
        return isdigit(c);
    case ParrotRegexExprType_NOT_DIGIT:
        return !isdigit(c);
    case ParrotRegexExprType_ALPHA:
        return isalnum(c);
    case ParrotRegexExprType_NOT_ALPHA:
        return !isalnum(c);
    case ParrotRegexExprType_WHITESPACE:
        return isspace(c);
    case ParrotRegexExprType_NOT_WHITESPACE:
        return !isspace(c);
    default:
        return (p.data.ch == c);
    }
}

static bool matchpattern(ParrotRegexExpr *pattern, const char *text, size_t *out_size);

static bool matchstar(ParrotRegexExpr p, ParrotRegexExpr *pattern, const char *text, size_t *out_size) {
    size_t prelen = *out_size;
    const char *prepoint = text;

    while ((text[0] != '\0') && matchone(p, *text)) {
        text++;
        (*out_size)++;
    }

    while (text >= prepoint) {
        if (matchpattern(pattern, text--, out_size)) {
            return true;
        }
        (*out_size)--;
    }

    *out_size = prelen;
    return false;
}

static bool matchplus(ParrotRegexExpr p, ParrotRegexExpr *pattern, const char *text, size_t *out_size) {
    const char *prepoint = text;

    while ((text[0] != '\0') && matchone(p, *text)) {
        text++;
        (*out_size)++;
    }

    while (text > prepoint) {
        if (matchpattern(pattern, text--, out_size)) {
            return true;
        }
        (*out_size)--;
    }

    return false;
}

static bool matchquestion(ParrotRegexExpr p, ParrotRegexExpr *pattern, const char *text, size_t *out_size) {
    if (p.type == ParrotRegexExprType_EOF)
        return true;
    if (matchpattern(pattern, text, out_size))
        return true;
    if (*text && matchone(p, *text++)) {
        if (matchpattern(pattern, text, out_size)) {
            (*out_size)++;
            return true;
        }
    }

    return false;
}

static bool matchpattern(ParrotRegexExpr *pattern, const char *text, size_t *out_size) {
    size_t pre = *out_size;
    do {
        if ((pattern[0].type == ParrotRegexExprType_EOF) || (pattern[1].type == ParrotRegexExprType_QUESTIONMARK)) {
            return matchquestion(pattern[0], &pattern[2], text, out_size);
        } else if (pattern[1].type == ParrotRegexExprType_STAR) {
            return matchstar(pattern[0], &pattern[2], text, out_size);
        } else if (pattern[1].type == ParrotRegexExprType_PLUS) {
            return matchplus(pattern[0], &pattern[2], text, out_size);
        } else if ((pattern[0].type == ParrotRegexExprType_END) && pattern[1].type == ParrotRegexExprType_EOF) {
            return (text[0] == '\0');
        }
        (*out_size)++;
    } while ((text[0] != '\0') && matchone(*pattern++, *text++));

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

    if (self->arr_expr[0].type == ParrotRegexExprType_BEGIN) {
        return matchpattern(&self->arr_expr[1], text, out_size);
    }

    return matchpattern(self->arr_expr, text, out_size);
}

ptrdiff_t ParrotRegex_match_all(ParrotRegex *self, const char *text, size_t *out_size) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(text);

    for (size_t i = 0; text[i] != '\0'; i++) {
        if (ParrotRegex_match(self, &text[i], out_size)) {
            return i;
        }
    }

    return -1;
}
