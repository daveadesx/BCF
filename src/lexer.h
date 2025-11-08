#ifndef LEXER_H
#define LEXER_H

#include "token.h"

/*
 * Lexer structure
 * Manages tokenization of source code
 */
typedef struct {
	char *source;
	int source_len;
	int pos;
	int line;
	int column;

	Token **tokens;
	int token_count;
	int token_capacity;

	int error_count;
} Lexer;

/* Lexer lifecycle */
Lexer *lexer_create(const char *source);
void lexer_destroy(Lexer *lexer);

/* Main tokenization */
int lexer_tokenize(Lexer *lexer);

/* Token access */
Token **lexer_get_tokens(Lexer *lexer);
int lexer_get_token_count(Lexer *lexer);

#endif /* LEXER_H */
