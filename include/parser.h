#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"

/*
 * Parser structure
 * Manages conversion of tokens to AST
 */
typedef struct {
	Token **tokens;
	int token_count;
	int current;

	int error_count;
} Parser;

/* Parser lifecycle */
Parser *parser_create(Token **tokens, int token_count);
void parser_destroy(Parser *parser);

/* Main parsing */
ASTNode *parser_parse(Parser *parser);

#endif /* PARSER_H */
