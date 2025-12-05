#define _GNU_SOURCE
#include "../include/lexer.h"
#include "../include/formatter.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Options structure */
typedef struct {
	int in_place;      /* -i: modify files in place */
	int check_only;    /* -c: check if formatted (don't modify) */
	int show_diff;     /* -d: show diff of changes */
	char *output_file; /* -o: output to specific file */
} Options;

/**
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

/**
 * print_version - Print version information
 */
static void print_version(void)
{
	printf("betty-fmt 0.1.0\n");
	printf("A Betty-compliant C code formatter\n");
}

/**
 * format_to_string - Format source code and return as string
 * @source: Source code to format
 * @out_len: Output parameter for result length
 *
 * Return: Formatted string (caller must free), or NULL on error
 */
static char *format_to_string(const char *source, size_t *out_len)
{
	Lexer *lexer;
	Formatter *formatter = NULL;
	char *result = NULL;
	FILE *mem_stream = NULL;
	size_t size = 0;
	int status = -1;

	lexer = lexer_create(source);
	if (!lexer)
		return (NULL);

	if (lexer_tokenize(lexer) < 0)
		goto cleanup;

	mem_stream = open_memstream(&result, &size);
	if (!mem_stream)
		goto cleanup;

	formatter = formatter_create(mem_stream);
	if (!formatter)
		goto cleanup;

	status = formatter_format(formatter,
				   lexer_get_tokens(lexer),
				   lexer_get_token_count(lexer));
	formatter_destroy(formatter);
	formatter = NULL;
	fclose(mem_stream);
	mem_stream = NULL;

cleanup:
	if (formatter)
		formatter_destroy(formatter);
	if (mem_stream)
		fclose(mem_stream);
	lexer_destroy(lexer);

	if (status < 0)
	{
		free(result);
		result = NULL;
		size = 0;
	}

	if (out_len)
		*out_len = size;

	return (result);
}

/**
 * do_write_file - Write content to a file (with length)
 * @filename: File to write
 * @content: Content to write
 * @len: Length of content
 *
 * Return: 0 on success, -1 on error
 */
static int do_write_file(const char *filename, const char *content, size_t len)
{
	FILE *fp;

	fp = fopen(filename, "w");
	if (!fp)
	{
		fprintf(stderr, "Error: Could not open '%s' for writing\n", filename);
		return (-1);
	}

	if (fwrite(content, 1, len, fp) != len)
	{
		fprintf(stderr, "Error: Failed to write to '%s'\n", filename);
		fclose(fp);
		return (-1);
	}

	fclose(fp);
	return (0);
}

/**
 * process_file - Process a single file
 * @filename: File to process
 * @opts: Processing options
 *
 * Return: 0 on success, 1 if needs formatting (check mode), -1 on error
 */
static int process_file(const char *filename, Options *opts)
{
	char *source;
	char *formatted;
	size_t formatted_len;
	int result = 0;

	source = read_file(filename);
	if (!source)
	{
		fprintf(stderr, "Error: Could not read file '%s'\n", filename);
		return (-1);
	}

	formatted = format_to_string(source, &formatted_len);
	if (!formatted)
	{
		fprintf(stderr, "Error: Failed to format '%s'\n", filename);
		free(source);
		return (-1);
	}

	/* Check mode: compare and report */
	if (opts->check_only)
	{
		if (strcmp(source, formatted) != 0)
		{
			printf("%s: needs formatting\n", filename);
			result = 1;
		}
		else
		{
			printf("%s: OK\n", filename);
		}
	}
	/* Diff mode: show differences */
	else if (opts->show_diff)
	{
		if (strcmp(source, formatted) != 0)
		{
			/* Write formatted to temp file and use diff */
			char temp_path[] = "/tmp/betty-fmt-XXXXXX";
			int temp_fd = mkstemp(temp_path);

			if (temp_fd >= 0)
			{
				FILE *temp_file = fdopen(temp_fd, "w");

				if (temp_file)
				{
					char cmd[512];

					fputs(formatted, temp_file);
					fclose(temp_file);
					snprintf(cmd, sizeof(cmd),
						"diff -u '%s' '%s' | head -100",
						filename, temp_path);
					system(cmd);
				}
				unlink(temp_path);
			}
			result = 1;
		}
		else
		{
			printf("%s: no changes\n", filename);
		}
	}
	/* In-place mode: write back to file */
	else if (opts->in_place)
	{
		if (strcmp(source, formatted) != 0)
		{
			if (do_write_file(filename, formatted, formatted_len) < 0)
				result = -1;
			else
				printf("Formatted: %s\n", filename);
		}
		else
		{
			/* File already formatted, no change needed */
		}
	}
	/* Output file mode */
	else if (opts->output_file)
	{
		if (do_write_file(opts->output_file, formatted, formatted_len) < 0)
			result = -1;
	}
	/* Default: print to stdout */
	else
	{
		printf("%s", formatted);
	}

	free(formatted);
	free(source);

	return (result);
}

/**
 * main - Entry point
 * @argc: Argument count
 * @argv: Argument vector
 *
 * Return: 0 on success, 1 on error or if files need formatting (check mode)
 */
int main(int argc, char **argv)
{
	Options opts = {0, 0, 0, NULL};
	int i;
	int file_count = 0;
	int error_count = 0;
	int needs_format = 0;

	if (argc < 2)
	{
		print_usage(argv[0]);
		return (1);
	}

	/* First pass: parse options */
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			print_usage(argv[0]);
			return (0);
		}
		else if (strcmp(argv[i], "-v") == 0 ||
			 strcmp(argv[i], "--version") == 0)
		{
			print_version();
			return (0);
		}
		else if (strcmp(argv[i], "-i") == 0 ||
			 strcmp(argv[i], "--in-place") == 0)
		{
			opts.in_place = 1;
		}
		else if (strcmp(argv[i], "-c") == 0 ||
			 strcmp(argv[i], "--check") == 0)
		{
			opts.check_only = 1;
		}
		else if (strcmp(argv[i], "-d") == 0 ||
			 strcmp(argv[i], "--diff") == 0)
		{
			opts.show_diff = 1;
		}
		else if (strcmp(argv[i], "-o") == 0 ||
			 strcmp(argv[i], "--output") == 0)
		{
			if (i + 1 < argc)
			{
				opts.output_file = argv[++i];
			}
			else
			{
				fprintf(stderr, "Error: -o requires a filename\n");
				return (1);
			}
		}
	}

	/* Second pass: process files */
	for (i = 1; i < argc; i++)
	{
		int ret;

		/* Skip options */
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-o") == 0 ||
			    strcmp(argv[i], "--output") == 0)
				i++; /* Skip the output filename too */
			continue;
		}

		file_count++;
		ret = process_file(argv[i], &opts);

		if (ret < 0)
			error_count++;
		else if (ret > 0)
			needs_format++;
	}

	if (file_count == 0)
	{
		fprintf(stderr, "Error: No input files\n");
		return (1);
	}

	if (error_count > 0)
		return (1);

	/* In check mode, return 1 if any files need formatting */
	if (opts.check_only && needs_format > 0)
		return (1);

	return (0);
}
