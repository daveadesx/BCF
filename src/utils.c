#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * read_file - Read entire file into string
 * @filename: Path to file
 *
 * Return: File contents, or NULL on error (caller must free)
 */
char *read_file(const char *filename)
{
	FILE *file;
	long file_size;
	char *buffer;
	size_t bytes_read;

	file = fopen(filename, "r");
	if (!file)
		return (NULL);

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = malloc(file_size + 1);
	if (!buffer)
	{
		fclose(file);
		return (NULL);
	}

	bytes_read = fread(buffer, 1, file_size, file);
	buffer[bytes_read] = '\0';

	fclose(file);
	return (buffer);
}

/*
 * write_file - Write string to file
 * @filename: Path to file
 * @content: Content to write
 *
 * Return: 0 on success, -1 on error
 */
int write_file(const char *filename, const char *content)
{
	FILE *file;
	size_t len, written;

	if (!filename || !content)
		return (-1);

	file = fopen(filename, "w");
	if (!file)
		return (-1);

	len = strlen(content);
	written = fwrite(content, 1, len, file);

	fclose(file);
	return (written == len ? 0 : -1);
}

/*
 * is_digit - Check if character is a digit
 * @c: Character to check
 *
 * Return: 1 if digit, 0 otherwise
 */
int is_digit(char c)
{
	return (c >= '0' && c <= '9');
}

/*
 * is_alpha - Check if character is alphabetic
 * @c: Character to check
 *
 * Return: 1 if alphabetic, 0 otherwise
 */
int is_alpha(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_');
}

/*
 * is_alnum - Check if character is alphanumeric
 * @c: Character to check
 *
 * Return: 1 if alphanumeric, 0 otherwise
 */
int is_alnum(char c)
{
	return (is_alpha(c) || is_digit(c));
}

/*
 * is_whitespace - Check if character is whitespace (not newline)
 * @c: Character to check
 *
 * Return: 1 if whitespace, 0 otherwise
 */
int is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\r');
}

/*
 * report_error - Print error message with location
 * @filename: Source filename
 * @line: Line number
 * @column: Column number
 * @message: Error message
 */
void report_error(const char *filename, int line, int column,
		  const char *message)
{
	fprintf(stderr, "%s:%d:%d: error: %s\n",
		filename ? filename : "<input>", line, column, message);
}
