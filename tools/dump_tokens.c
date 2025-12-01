/*
 * dump_tokens.c - Debug tool to print lexer token output
 */
#include "../include/lexer.h"
#include "../include/token.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * main - Tokenize a file and print all tokens
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Return: 0 on success, 1 on error
 */
int main(int argc, char **argv)
{
	char *source;
	Lexer *lexer;
	Token **tokens;
	int count, i;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <file.c>\n", argv[0]);
		return (1);
	}

	source = read_file(argv[1]);
	if (!source)
	{
		fprintf(stderr, "Error: Could not read file '%s'\n", argv[1]);
		return (1);
	}

	lexer = lexer_create(source);
	if (!lexer)
	{
		fprintf(stderr, "Error: Failed to create lexer\n");
		free(source);
		return (1);
	}

	if (lexer_tokenize(lexer) < 0)
	{
		fprintf(stderr, "Error: Tokenization failed\n");
		lexer_destroy(lexer);
		free(source);
		return (1);
	}

	tokens = lexer_get_tokens(lexer);
	count = lexer_get_token_count(lexer);

	printf("=== Tokens for %s ===\n", argv[1]);
	printf("Total tokens: %d\n\n", count);

	for (i = 0; i < count; i++)
	{
		Token *t = tokens[i];
		printf("[%3d] %-20s  line:%-3d col:%-3d  \"%s\"\n",
		       i,
		       token_type_to_string(t->type),
		       t->line,
		       t->column,
		       t->lexeme ? t->lexeme : "(null)");
	}

	lexer_destroy(lexer);
	free(source);

	return (0);
}
