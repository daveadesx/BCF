#include "lexer.h"
#include "parser.h"
#include "formatter.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * print_usage - Print usage information
 * @program: Program name
 */
static void print_usage(const char *program)
{
	printf("Usage: %s [options] <files...>\n\n", program);
	printf("Options:\n");
	printf("  -i, --in-place      Modify files in place\n");
	printf("  -o, --output FILE   Write to FILE instead of stdout\n");
	printf("  -c, --check         Check if files are formatted (exit 1 if not)\n");
	printf("  -d, --diff          Show diff of changes\n");
	printf("  -h, --help          Show this help message\n");
	printf("  -v, --version       Show version\n\n");
	printf("Examples:\n");
	printf("  %s main.c                    Print formatted to stdout\n", program);
	printf("  %s -i *.c                    Format all .c files in place\n", program);
	printf("  %s -c src/*.c                Check if files need formatting\n", program);
}

/*
 * print_version - Print version information
 */
static void print_version(void)
{
	printf("betty-fmt 0.1.0\n");
	printf("A Betty-compliant C code formatter\n");
}

/*
 * process_file - Process a single file
 * @filename: File to process
 *
 * Return: 0 on success, -1 on error
 */
static int process_file(const char *filename)
{
	char *source;
	Lexer *lexer;
	Parser *parser;
	int result = 0;

	source = read_file(filename);
	if (!source)
	{
		fprintf(stderr, "Error: Could not read file '%s'\n", filename);
		return (-1);
	}

	lexer = lexer_create(source);
	if (!lexer)
	{
		fprintf(stderr, "Error: Failed to create lexer\n");
		free(source);
		return (-1);
	}

	if (lexer_tokenize(lexer) < 0)
	{
		fprintf(stderr, "Error: Tokenization failed\n");
		lexer_destroy(lexer);
		free(source);
		return (-1);
	}

	parser = parser_create(lexer_get_tokens(lexer),
			       lexer_get_token_count(lexer));
	if (!parser)
	{
		fprintf(stderr, "Error: Failed to create parser\n");
		lexer_destroy(lexer);
		free(source);
		return (-1);
	}

	/* TODO: Parse and format */

	parser_destroy(parser);
	lexer_destroy(lexer);
	free(source);

	return (result);
}

/*
 * main - Entry point
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Return: 0 on success, 1 on error
 */
int main(int argc, char **argv)
{
	int i;

	if (argc < 2)
	{
		print_usage(argv[0]);
		return (1);
	}

	/* Handle options */
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			print_usage(argv[0]);
			return (0);
		}
		else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
		{
			print_version();
			return (0);
		}
		else
		{
			/* Process file */
			if (process_file(argv[i]) < 0)
				return (1);
		}
	}

	return (0);
}
