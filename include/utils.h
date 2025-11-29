#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/* File I/O utilities */
char *read_file(const char *filename);
int write_file(const char *filename, const char *content);

/* String utilities */
int is_digit(char c);
int is_alpha(char c);
int is_alnum(char c);
int is_whitespace(char c);

/* Error reporting */
void report_error(const char *filename, int line, int column,
		  const char *message);

#endif /* UTILS_H */
