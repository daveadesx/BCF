#define _GNU_SOURCE
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
	char *result = NULL;
	FILE *mem_stream;
	size_t size = 0;

	if (!source)
		return (NULL);

	mem_stream = open_memstream(&result, &size);
	if (!mem_stream)
		return (NULL);

	if (format_source(source, mem_stream) < 0)
	{
		fclose(mem_stream);
		free(result);
		return (NULL);
	}

	fclose(mem_stream);

	if (out_len)
		*out_len = size;

	return (result);
}

/**
 * files_differ - Check if two strings differ
 * @s1: First string
 * @s2: Second string
 *
 * Return: 1 if different, 0 if same
 */
static int files_differ(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return (1);

	return (strcmp(s1, s2) != 0);
}

/**
 * show_diff - Print diff between original and formatted
 * @filename: File name for header
 * @original: Original content
 * @formatted: Formatted content
 */
static void show_diff(const char *filename, const char *original,
		      const char *formatted)
{
	FILE *orig_file, *fmt_file;
	char orig_path[] = "/tmp/betty-fmt-orig.XXXXXX";
	char fmt_path[] = "/tmp/betty-fmt-fmt.XXXXXX";
	char cmd[512];
	int orig_fd, fmt_fd;

	orig_fd = mkstemp(orig_path);
	fmt_fd = mkstemp(fmt_path);

	if (orig_fd < 0 || fmt_fd < 0)
	{
		fprintf(stderr, "Error creating temp files for diff\n");
		return;
	}

	orig_file = fdopen(orig_fd, "w");
	fmt_file = fdopen(fmt_fd, "w");

	if (orig_file && fmt_file)
	{
		fputs(original, orig_file);
		fputs(formatted, fmt_file);
		fclose(orig_file);
		fclose(fmt_file);

		snprintf(cmd, sizeof(cmd), "diff -u --label '%s (original)' "
			 "--label '%s (formatted)' '%s' '%s'",
			 filename, filename, orig_path, fmt_path);
		system(cmd);
	}

	unlink(orig_path);
	unlink(fmt_path);
}

/**
 * process_file - Process a single file
 * @filename: File to process
 * @opts: Processing options
 *
 * Return: 0 on success, 1 if file needs formatting (check mode), -1 on error
 */
static int process_file(const char *filename, Options *opts)
{
	char *source;
	char *formatted;
	size_t fmt_len;
	FILE *output;
	int result = 0;

	source = read_file(filename);
	if (!source)
	{
		fprintf(stderr, "Error reading %s\n", filename);
		return (-1);
	}

	formatted = format_to_string(source, &fmt_len);
	if (!formatted)
	{
		fprintf(stderr, "Error formatting %s\n", filename);
		free(source);
		return (-1);
	}

	if (opts->check_only)
	{
		if (files_differ(source, formatted))
		{
			printf("%s needs formatting\n", filename);
			result = 1;
		}
	}
	else if (opts->show_diff)
	{
		if (files_differ(source, formatted))
			show_diff(filename, source, formatted);
	}
	else if (opts->in_place)
	{
		if (files_differ(source, formatted))
		{
			output = fopen(filename, "w");
			if (!output)
			{
				fprintf(stderr, "Error writing %s\n", filename);
				result = -1;
			}
			else
			{
				fputs(formatted, output);
				fclose(output);
				printf("Formatted %s\n", filename);
			}
		}
	}
	else if (opts->output_file)
	{
		output = fopen(opts->output_file, "w");
		if (!output)
		{
			fprintf(stderr, "Error writing %s\n", opts->output_file);
			result = -1;
		}
		else
		{
			fputs(formatted, output);
			fclose(output);
		}
	}
	else
	{
		/* Output to stdout */
		printf("%s", formatted);
	}

	free(source);
	free(formatted);
	return (result);
}

/**
 * main - Entry point
 * @argc: Argument count
 * @argv: Arguments
 *
 * Return: 0 on success, non-zero on error
 */
int main(int argc, char *argv[])
{
	Options opts = {0};
	int i;
	int file_count = 0;
	int result = 0;
	int file_result;

	/* Parse arguments */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-h") == 0 ||
			    strcmp(argv[i], "--help") == 0)
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
			else if ((strcmp(argv[i], "-o") == 0 ||
				  strcmp(argv[i], "--output") == 0) &&
				 i + 1 < argc)
			{
				opts.output_file = argv[++i];
			}
			else
			{
				fprintf(stderr, "Unknown option: %s\n", argv[i]);
				return (1);
			}
		}
	}

	/* Process files */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-' &&
		    (i == 1 || strcmp(argv[i - 1], "-o") != 0))
		{
			file_count++;
			file_result = process_file(argv[i], &opts);
			if (file_result < 0)
				result = 1;
			else if (file_result > 0 && opts.check_only)
				result = 1;
		}
	}

	if (file_count == 0)
	{
		print_usage(argv[0]);
		return (1);
	}

	return (result);
}
