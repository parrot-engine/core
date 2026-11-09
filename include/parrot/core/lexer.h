#ifndef PARROT_CORE_INCLUDE_PARROT_CORE_LEXER_H_
#define PARROT_CORE_INCLUDE_PARROT_CORE_LEXER_H_

#include "parrot/core/regex.h"
#include "parrot/core/util.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    ParrotLexerRuleType_END = 0,

    ParrotLexerRuleType_REGEX,
    ParrotLexerRuleType_FUNC,
} ParrotLexerRuleType;

typedef struct {
    ParrotLexerRuleType type;

    /// <0 = do not push anything
    int token;

    union {
        struct {
            ParrotRegex *regex;
            /// Deletes after construction if false
            bool borrow;
        } regex;

        struct {
            /// @returns Amount to advance by or 0 for no match
            size_t (*match)(void *ctx, const char *str);
            void *ctx;
        } func;
    } data;
} ParrotLexerRule;

#define PARROT_LEXER_RULE_END(p_other_token)                                                                            \
    {                                                                                                                   \
        .type = ParrotLexerRuleType_END,                                                                                \
        .token = p_other_token,                                                                                         \
    }

#define PARROT_LEXER_RULE_REGEX_TOKEN(p_token, p_regex, p_borrow)                                                       \
    {                                                                                                                   \
        .type = ParrotLexerRuleType_REGEX,                                                                              \
        .token = p_token,                                                                                               \
        .data =                                                                                                         \
            {                                                                                                           \
                .regex =                                                                                                \
                    {                                                                                                   \
                        .regex = p_regex,                                                                               \
                        .borrow = p_borrow,                                                                             \
                    },                                                                                                  \
            },                                                                                                          \
    }

#define PARROT_LEXER_RULE_FUNC_TOKEN(p_token, p_match, p_ctx)                                                           \
    {                                                                                                                   \
        .type = ParrotLexerRuleType_FUNC,                                                                               \
        .token = p_token,                                                                                               \
        .data =                                                                                                         \
            {                                                                                                           \
                .func =                                                                                                 \
                    {                                                                                                   \
                        .match = p_match,                                                                               \
                        .ctx = p_ctx,                                                                                   \
                    },                                                                                                  \
            },                                                                                                          \
    }

typedef struct ParrotLexer ParrotLexer;

typedef struct {
    int token;

    const char *position;
    size_t size;
} ParrotLexerToken;

PARROT_API ParrotLexer *ParrotLexer_new(int eof_token);
PARROT_API void ParrotLexer_delete(ParrotLexer *self);
PARROT_API void ParrotLexer_vdelete(void *self);

PARROT_API void
ParrotLexer_lex(ParrotLexer *self, /* First rule wins */ const ParrotLexerRule *ruleset, const char *str);

PARROT_API void ParrotLexer_push(ParrotLexer *self, ParrotLexerToken token);
PARROT_API void ParrotLexer_insert(ParrotLexer *self, size_t position, ParrotLexerToken token);
PARROT_API void ParrotLexer_remove(ParrotLexer *self, size_t position);

PARROT_API size_t ParrotLexer_get_position(ParrotLexer *self);
PARROT_API void ParrotLexer_set_position(ParrotLexer *self, size_t position);

/// Returns the EOF token if out of bounds
PARROT_API ParrotLexerToken ParrotLexer_pop(ParrotLexer *self);
/**
 * Peek N tokens ahead where 0 is the next token.
 * Returns the EOF token if out of bounds.
 */
PARROT_API ParrotLexerToken ParrotLexer_peek(ParrotLexer *self, size_t n);

#endif // PARROT_CORE_INCLUDE_PARROT_CORE_LEXER_H_
