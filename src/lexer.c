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

typedef struct {
    ParrotRegex *regex;
} LexerContextRuleCacheEntry;

typedef struct {
    ParrotScope *scope;

    const ParrotLexerRule *ruleset;

    LexerContextRuleCacheEntry rule_cache[];
} LexerContext;

static size_t lexer_rule_match_count_or_zero(LexerContext *ctx, size_t rule_index, const char *str) {
    const ParrotLexerRule *rule = &ctx->ruleset[rule_index];

    switch (rule->type) {
    case ParrotLexerRuleType_END:
        return 1;
    case ParrotLexerRuleType_REGEX: {
        if (!ctx->rule_cache[rule_index].regex) {
            ctx->rule_cache[rule_index].regex = ParrotRegex_new_or_null(rule->data.regex);
            ParrotScope_push(ctx->scope, ParrotRegex_vdelete, ctx->rule_cache[rule_index].regex);
        }

        size_t match_size = 0;
        return ParrotRegex_match(ctx->rule_cache[rule_index].regex, str, &match_size) ? match_size : 0;
    }
    case ParrotLexerRuleType_REGEX_OBJECT: {
        size_t match_size = 0;
        return ParrotRegex_match(rule->data.regex_object, str, &match_size) ? match_size : 0;
    }
    case ParrotLexerRuleType_FUNC:
        return rule->data.func.match_count_or_zero(rule->data.func.ctx, str);
    }
    return 0;
}

void ParrotLexer_lex(ParrotLexer *self, const ParrotLexerRule *ruleset, const char *str) {
    PARROT_FAIL_NULL(self);
    PARROT_FAIL_NULL(ruleset);
    PARROT_FAIL_NULL(str);

    size_t rule_count = 0;
    for (const ParrotLexerRule *rule = ruleset; rule->type != ParrotLexerRuleType_END; rule++) {
        rule_count++;
    }

    size_t cache_size = rule_count * sizeof(LexerContextRuleCacheEntry);

    LexerContext *ctx = malloc(sizeof(LexerContext) + cache_size);
    memset(ctx, 0, sizeof(LexerContext) + cache_size);

    ctx->scope = ParrotScope_new(NULL);
    ParrotScope_push_free(ctx->scope, ctx);

    ctx->ruleset = ruleset;

    while (*str) {
        size_t rule_index = 0;

        const char *match = str;
        size_t match_size = 0;

        for (;;) {
            match_size = lexer_rule_match_count_or_zero(ctx, rule_index, str);
            if (match_size > 0) {
                str += match_size;
                break;
            }

            // An END token returns 1
            rule_index++;
        }

        const ParrotLexerRule *rule = &ruleset[rule_index];

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

    ParrotScope_delete(ctx->scope);
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

size_t ParrotLexer_get_position(ParrotLexer *self) {
    PARROT_FAIL_NULL(self);

    return self->stream_position;
}

void ParrotLexer_set_position(ParrotLexer *self, size_t position) {
    PARROT_FAIL_NULL(self);

    self->stream_position = position;
}

ParrotLexerToken ParrotLexer_pop_or_eof(ParrotLexer *self) {
    PARROT_FAIL_NULL(self);

    ParrotLexerToken token = ParrotLexer_peek_or_eof(self, 0);
    self->stream_position++;
    return token;
}

ParrotLexerToken ParrotLexer_peek_or_eof(ParrotLexer *self, size_t n) {
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
