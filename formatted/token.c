#define _POSIX_C_SOURCE 200809L
#include "../include/token.h"
#include <stdlib.h>
#include <string.h>

/*
 * token_create - Create a new token
 * @type: Token type
 * @lexeme: Token text (will be copied)
 * @line: Line number
 * @column: Column number
 *
 * Return: Pointer to new token, or NULL on failure
 */
Token *token_create(TokenType type, const char *lexeme, int line, int column)
{
	Token *token;

	token = malloc(sizeof(Token));
	if (!token)
		return (NULL);
	token->type = type;
	token->lexeme = lexeme ? strdup(lexeme) : NULL;
	token->line = line;
	token->column = column;
	token->length = lexeme ? strlen(lexeme) : 0;

	return (token);
}

/*
 * token_destroy - Free token memory
 * @token: Token to destroy
 */
void token_destroy(Token *token)
{
	if (!token)
		return;
	free(token->lexeme);
	free(token);
}

/*
 * token_type_to_string - Convert token type to string for debugging
 * @type: Token type
 *
 * Return: String representation of token type
 */
const char *token_type_to_string(TokenType type)
{
	static const char *names[] = {"WHITESPACE", "NEWLINE", "COMMENT_LINE", "COMMENT_BLOCK", "PREPROCESSOR", "IDENTIFIER", "INTEGER", "FLOAT", "STRING", "CHAR", "IF", "ELSE", "WHILE", "FOR", "DO", "SWITCH", "CASE", "DEFAULT", "BREAK", "CONTINUE", "RETURN", "GOTO", "TYPEDEF", "STRUCT", "UNION", "ENUM", "SIZEOF", "VOID", "CHAR_KW", "SHORT", "INT", "LONG", "FLOAT_KW", "DOUBLE", "SIGNED", "UNSIGNED", "CONST", "VOLATILE", "STATIC", "EXTERN", "AUTO", "REGISTER", "PLUS", "MINUS", "STAR", "SLASH", "PERCENT", "ASSIGN", "PLUS_ASSIGN", "MINUS_ASSIGN", "STAR_ASSIGN", "SLASH_ASSIGN", "PERCENT_ASSIGN", "AMPERSAND_ASSIGN", "PIPE_ASSIGN", "CARET_ASSIGN", "LSHIFT_ASSIGN", "RSHIFT_ASSIGN", "EQUAL", "NOT_EQUAL", "LESS", "GREATER", "LESS_EQUAL", "GREATER_EQUAL", "LOGICAL_AND", "LOGICAL_OR", "LOGICAL_NOT", "AMPERSAND", "PIPE", "CARET", "TILDE", "LSHIFT", "RSHIFT", "INCREMENT", "DECREMENT", "ARROW", "DOT", "ELLIPSIS", "QUESTION", "COLON", "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACKET", "RBRACKET", "SEMICOLON", "COMMA", "EOF", "ERROR"};

	if (type < 0 || type > TOK_ERROR)
		return ("UNKNOWN");
	return (names[type]);
}
