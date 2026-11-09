#include "parrot/core/lexer.h"
#include "parrot/core/array.h"
#include "parrot/core/regex.h"
#include "parrot/core/scope.h"

struct ParrotLexer {
    ParrotScope *scope;

    int eof_token;

    ParrotLexerToken *arr_stream;
    size_t stream_position;
};

ParrotLexer *ParrotLexer_new(int eof_token) {
    ParrotLexer *self = PARROT_ALLOC(ParrotLexer);

    self->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(self->scope, self);

    ParrotScope_push_arrfree(self->scope, self->arr_stream);

    self->eof_token = eof_token;

    return self;
}

void ParrotLexer_delete(ParrotLexer *self) {
    PARROT_FAIL_NULL(self);

    ParrotScope_delete(self->scope);
}

void ParrotLexer_vdelete(void *self) {
    ParrotLexer_delete(self);
}

static size_t rule_matches(const ParrotLexerRule *rule, const char *str) {
    switch (rule->type) {
    case ParrotLexerRuleType_END:
        return 1;
    case ParrotLexerRuleType_REGEX: {
        size_t match_size = 0;
        ptrdiff_t match_position = ParrotRegex_match(rule->data.regex.regex, str, &match_size);
        return match_position == 0 ? match_size : 0;
    }
    case ParrotLexerRuleType_FUNC:
        return rule->data.func.match(rule->data.func.ctx, str);
    }
    return 0;
}

void ParrotLexer_lex(ParrotLexer *self, const ParrotLexerRule *ruleset, const char *str) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(ruleset);
    PARROT_FAIL_NULL(str);

    while (*str) {
        const ParrotLexerRule *rule = ruleset;

        const char *match = str;
        size_t match_size = 0;

        for (;;) {
            match_size = rule_matches(rule, str);
            if (match_size > 0) {
                str += match_size;
                break;
            }

            // An END token returns 1
            rule++;
        }

        if (rule->token < 0) {
            continue;
        }

        ParrotLexer_push(self,
                         (ParrotLexerToken){
                             .token = rule->token,
                             .position = match,
                             .size = match_size,
                         });
    }

    for (const ParrotLexerRule *rule = ruleset; rule->type != ParrotLexerRuleType_END; rule++) {
        if (rule->type != ParrotLexerRuleType_REGEX) {
            continue;
        }

        if (rule->data.regex.borrow) {
            continue;
        }

        ParrotRegex_delete(rule->data.regex.regex);
    }
}

void ParrotLexer_push(ParrotLexer *self, ParrotLexerToken token) {
    PARROT_FAIL_NULL(self);

    ParrotArray_push(self->arr_stream, token);
}

void ParrotLexer_insert(ParrotLexer *self, size_t position, ParrotLexerToken token) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_COND(position > ParrotArray_size(self->arr_stream));

    ParrotArray_alloc_at(self->arr_stream, position, 1);
    self->arr_stream[position] = token;

    if (self->stream_position >= position) {
        self->stream_position++;
    }
}

void ParrotLexer_remove(ParrotLexer *self, size_t position) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_COND(position > ParrotArray_size(self->arr_stream));

    if (self->stream_position > position) {
        self->stream_position--;
    }

    ParrotArray_del(self->arr_stream, position);
}

ParrotLexerToken ParrotLexer_pop(ParrotLexer *self) {
    PARROT_FAIL_NULL(self);

    ParrotLexerToken token = ParrotLexer_peek(self, 0);
    self->stream_position++;
    return token;
}

ParrotLexerToken ParrotLexer_peek(ParrotLexer *self, size_t n) {
    PARROT_FAIL_NULL(self);

    size_t position = self->stream_position + n;
    if (position >= ParrotArray_size(self->arr_stream)) {
        return ((ParrotLexerToken){
            .token = self->eof_token,
            .position = NULL,
            .size = 1,
        });
    }

    return self->arr_stream[position];
}
