#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include "symbol_table.h"

/*
 * Parser structure
 * Manages conversion of tokens to AST
 */
typedef struct {
	Token **tokens;
	int token_count;
	int current;

	int error_count;
	int whitespace_start;

	SymbolTable *symbols;  /* Symbol table for typedef tracking */

	/* Comment collection buffer */
	Token **pending_comments;
	int pending_comment_count;
	int pending_comment_capacity;

	/* Trailing comment tracking */
	int last_token_line;  /* Line of last consumed significant token */
} Parser;

/* Parser lifecycle */
Parser *parser_create(Token **tokens, int token_count);
void parser_destroy(Parser *parser);

/* Main parsing */
ASTNode *parser_parse(Parser *parser);

#endif /* PARSER_H */
