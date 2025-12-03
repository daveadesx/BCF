#define _POSIX_C_SOURCE 200809L

#include "../include/formatter.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAB_WIDTH 8

/*
 * Line buffer for building output
 */
typedef struct {
	char **lines;
	int count;
	int capacity;
} LineBuffer;

static LineBuffer *linebuf_create(void)
{
	LineBuffer *lb = malloc(sizeof(LineBuffer));

	if (!lb)
		return (NULL);
	lb->lines = NULL;
	lb->count = 0;
	lb->capacity = 0;
	return (lb);
}

static void linebuf_destroy(LineBuffer *lb)
{
	int i;

	if (!lb)
		return;
	for (i = 0; i < lb->count; i++)
		free(lb->lines[i]);
	free(lb->lines);
	free(lb);
}

static int linebuf_add(LineBuffer *lb, const char *line)
{
	char **new_lines;

	if (lb->count >= lb->capacity)
	{
		lb->capacity = lb->capacity ? lb->capacity * 2 : 64;
		new_lines = realloc(lb->lines, lb->capacity * sizeof(char *));
		if (!new_lines)
			return (-1);
		lb->lines = new_lines;
	}
	lb->lines[lb->count] = strdup(line);
	if (!lb->lines[lb->count])
		return (-1);
	lb->count++;
	return (0);
}

/*
 * strip_trailing_whitespace - Remove trailing spaces/tabs
 */
static void strip_trailing(char *line)
{
	size_t len = strlen(line);

	while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' ||
			   line[len - 1] == '\r'))
		len--;
	line[len] = '\0';
}

/*
 * apply_indent - Apply a specific indent level (in tabs) to a line
 */
static char *apply_indent(const char *line, int indent_level)
{
	const char *p = line;
	size_t rest_len;
	char *result, *out;
	int i;

	/* Skip existing whitespace */
	while (*p == ' ' || *p == '\t')
		p++;

	rest_len = strlen(p);
	result = malloc(indent_level + rest_len + 1);
	if (!result)
		return (NULL);

	out = result;
	for (i = 0; i < indent_level; i++)
		*out++ = '\t';
	strcpy(out, p);
	return (result);
}

/*
 * get_trimmed - Get pointer to first non-whitespace char
 */
static const char *get_trimmed(const char *line)
{
	while (*line == ' ' || *line == '\t')
		line++;
	return (line);
}

/*
 * is_preprocessor - Check if line is a preprocessor directive
 */
static int is_preprocessor(const char *line)
{
	const char *p = get_trimmed(line);

	return (*p == '#');
}

/*
 * get_pp_group - Get preprocessor group type for blank line logic
 * Returns: 1=include, 2=define, 3=conditional, 4=other
 */
static int get_pp_group(const char *line)
{
	const char *p = get_trimmed(line);

	if (*p != '#')
		return (0);
	p++;
	while (*p == ' ' || *p == '\t')
		p++;

	if (strncmp(p, "include", 7) == 0)
		return (1);
	if (strncmp(p, "define", 6) == 0 || strncmp(p, "undef", 5) == 0)
		return (2);
	if (strncmp(p, "if", 2) == 0 || strncmp(p, "elif", 4) == 0 ||
	    strncmp(p, "else", 4) == 0 || strncmp(p, "endif", 5) == 0)
		return (3);
	return (4);
}

/*
 * is_empty_line - Check if line is empty or whitespace-only
 */
static int is_empty_line(const char *line)
{
	const char *p = get_trimmed(line);

	return (*p == '\0');
}

/*
 * starts_with_brace - Check if line starts with {
 */
static int starts_with_brace(const char *line)
{
	const char *p = get_trimmed(line);

	return (*p == '{');
}

/*
 * ends_with_open_brace - Check if line ends with {
 */
static int ends_with_open_brace(const char *line)
{
	size_t len = strlen(line);

	while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'))
		len--;
	return (len > 0 && line[len - 1] == '{');
}

/*
 * is_do_statement - Check if line is 'do' or 'do {'
 */
static int is_do_statement(const char *line)
{
	const char *p = get_trimmed(line);

	if (strncmp(p, "do", 2) != 0)
		return (0);
	p += 2;
	return (*p == '\0' || *p == ' ' || *p == '\t' || *p == '{');
}

/*
 * split_brace - Split a line that ends with { into two lines
 * For "int main(void) {" -> "int main(void)" and "{"
 * Returns the part before brace (caller must free), stores indent
 */
static char *split_brace(const char *line, char **indent_out)
{
	size_t len = strlen(line);
	const char *brace_pos;
	char *before;
	const char *p = line;
	size_t indent_len = 0;

	/* Find the { */
	while (len > 0 && line[len - 1] != '{')
		len--;
	if (len == 0)
		return (NULL);

	brace_pos = line + len - 1;

	/* Get indent */
	while (*p == ' ' || *p == '\t')
	{
		indent_len++;
		p++;
	}

	*indent_out = malloc(indent_len + 1);
	if (*indent_out)
	{
		memcpy(*indent_out, line, indent_len);
		(*indent_out)[indent_len] = '\0';
	}

	/* Copy part before brace */
	before = malloc(brace_pos - line + 1);
	if (!before)
		return (NULL);
	memcpy(before, line, brace_pos - line);
	before[brace_pos - line] = '\0';
	strip_trailing(before);

	return (before);
}

/*
 * is_function_def - Check if line looks like start of function definition
 * Has return type, name, params ending with )
 */
static int is_function_def(const char *line)
{
	const char *p = get_trimmed(line);
	const char *paren;
	size_t len;

	/* Should not start with keywords that aren't types */
	if (strncmp(p, "if", 2) == 0 || strncmp(p, "while", 5) == 0 ||
	    strncmp(p, "for", 3) == 0 || strncmp(p, "switch", 6) == 0 ||
	    strncmp(p, "return", 6) == 0)
		return (0);

	/* Must have ( and end with ) */
	paren = strchr(p, '(');
	if (!paren)
		return (0);

	len = strlen(p);
	while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t'))
		len--;

	return (len > 0 && p[len - 1] == ')');
}

/*
 * is_var_decl - Check if line is a variable declaration
 */
static int is_var_decl(const char *line)
{
	const char *p = get_trimmed(line);
	size_t len = strlen(p);

	/* Must end with ; */
	if (len == 0 || p[len - 1] != ';')
		return (0);

	/* Must not have ( - that would be function call or prototype */
	if (strchr(p, '('))
		return (0);

	/* Must start with type keyword */
	if (strncmp(p, "int ", 4) == 0 || strncmp(p, "char ", 5) == 0 ||
	    strncmp(p, "void ", 5) == 0 || strncmp(p, "long ", 5) == 0 ||
	    strncmp(p, "short ", 6) == 0 || strncmp(p, "float ", 6) == 0 ||
	    strncmp(p, "double ", 7) == 0 || strncmp(p, "unsigned ", 9) == 0 ||
	    strncmp(p, "signed ", 7) == 0 || strncmp(p, "static ", 7) == 0 ||
	    strncmp(p, "extern ", 7) == 0 || strncmp(p, "const ", 6) == 0 ||
	    strncmp(p, "size_t ", 7) == 0 || strncmp(p, "ssize_t ", 8) == 0)
		return (1);

	return (0);
}

/*
 * is_closing_brace_line - Check if line is just } or }; or similar
 */
static int is_closing_brace_line(const char *line)
{
	const char *p = get_trimmed(line);

	return (*p == '}');
}

/*
 * count_braces - Count net brace change in a line
 * Returns positive for more { than }, negative for more } than {
 */
static int count_braces(const char *line)
{
	int count = 0;
	int in_string = 0;
	int in_char = 0;
	char prev = 0;

	while (*line)
	{
		if (*line == '"' && prev != '\\' && !in_char)
			in_string = !in_string;
		else if (*line == '\'' && prev != '\\' && !in_string)
			in_char = !in_char;
		else if (!in_string && !in_char)
		{
			if (*line == '{')
				count++;
			else if (*line == '}')
				count--;
		}
		prev = *line;
		line++;
	}
	return (count);
}

/*
 * format_source - Format C source code
 */
int format_source(const char *source, FILE *output)
{
	LineBuffer *lb;
	const char *p = source;
	const char *end = source + strlen(source);
	const char *line_start;
	char *line, *indented, *before, *indent_str;
	int prev_pp_group = 0;
	int curr_pp_group;
	int prev_was_empty = 0;
	int prev_was_func_def = 0;
	int prev_was_var_decl = 0;
	int prev_was_closing_brace = 0;
	int i;
	int pending_blank = 0;
	int indent_level = 0;
	int brace_delta;
	const char *trimmed;

	lb = linebuf_create();
	if (!lb)
		return (-1);

	while (p < end)
	{
		/* Get line */
		line_start = p;
		while (p < end && *p != '\n')
			p++;

		line = malloc(p - line_start + 1);
		if (!line)
		{
			linebuf_destroy(lb);
			return (-1);
		}
		memcpy(line, line_start, p - line_start);
		line[p - line_start] = '\0';

		if (p < end)
			p++; /* skip newline */

		/* Strip trailing whitespace */
		strip_trailing(line);

		trimmed = get_trimmed(line);

		/* Handle empty lines - collapse multiples */
		if (*trimmed == '\0')
		{
			if (!prev_was_empty && lb->count > 0)
			{
				linebuf_add(lb, "");
				prev_was_empty = 1;
			}
			free(line);
			continue;
		}

		prev_was_empty = 0;

		/* Check for blank line between different PP groups */
		if (is_preprocessor(line))
		{
			curr_pp_group = get_pp_group(line);
			if (prev_pp_group != 0 && curr_pp_group != prev_pp_group)
				pending_blank = 1;
			prev_pp_group = curr_pp_group;
		}
		else
		{
			/* Blank after PP block before code */
			if (prev_pp_group != 0)
				pending_blank = 1;
			prev_pp_group = 0;
		}

		/* Blank after function definition header (before opening brace) */
		if (prev_was_func_def && starts_with_brace(line))
			pending_blank = 0; /* Actually no blank here */

		/* Blank after variable declarations at function start */
		if (prev_was_var_decl && !is_var_decl(line) &&
		    !is_empty_line(line) && !starts_with_brace(line))
			pending_blank = 1;

		/* Blank after closing brace of struct/function */
		if (prev_was_closing_brace && !is_empty_line(line) &&
		    !is_preprocessor(line) && *trimmed != '}')
			pending_blank = 1;

		/* Add pending blank line if needed */
		if (pending_blank && lb->count > 0)
		{
			/* Check last line isn't already blank */
			if (lb->lines[lb->count - 1][0] != '\0')
				linebuf_add(lb, "");
			pending_blank = 0;
		}

		/* Calculate brace delta for this line */
		brace_delta = count_braces(line);

		/* Decrease indent BEFORE line if it starts with } */
		if (*trimmed == '}' && indent_level > 0)
			indent_level--;

		/* Handle brace on same line - move to new line */
		/* Exception: do { should keep brace on same line */
		if (ends_with_open_brace(line) && !is_do_statement(line) &&
		    !starts_with_brace(line))
		{
			before = split_brace(line, &indent_str);
			if (before && strlen(get_trimmed(before)) > 0)
			{
				/* Apply indent to the line before brace */
				indented = apply_indent(before, indent_level);
				free(before);
				if (indented)
				{
					linebuf_add(lb, indented);
					free(indented);
				}
				free(indent_str);

				/* Add { on new line with same indent */
				indented = apply_indent("{", indent_level);
				if (indented)
				{
					linebuf_add(lb, indented);
					free(indented);
				}

				/* Increase indent after { */
				indent_level++;

				free(line);
				prev_was_func_def = 0;
				prev_was_var_decl = 0;
				prev_was_closing_brace = 0;
				continue;
			}
			free(before);
			free(indent_str);
		}

		/* Apply proper indentation */
		if (!is_preprocessor(line))
		{
			indented = apply_indent(line, indent_level);
			free(line);
			line = indented;
		}

		/* Increase indent AFTER line if it has more { than } */
		/* But only if we didn't already handle brace splitting */
		if (brace_delta > 0)
			indent_level += brace_delta;

		/* Track state for next iteration */
		prev_was_func_def = is_function_def(line);
		prev_was_var_decl = is_var_decl(line);
		prev_was_closing_brace = is_closing_brace_line(line);

		linebuf_add(lb, line);
		free(line);
	}

	/* Output all lines */
	for (i = 0; i < lb->count; i++)
		fprintf(output, "%s\n", lb->lines[i]);

	linebuf_destroy(lb);
	return (0);
}
