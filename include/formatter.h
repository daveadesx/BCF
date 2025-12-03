#ifndef FORMATTER_H
#define FORMATTER_H

#include "ast.h"
#include <stdio.h>

/*
 * Formatter structure
 * Manages pretty-printing of AST to formatted code
 */
typedef struct {
	FILE *output;
	int indent_level;
	int column;
	int line;
	int at_line_start;

	/* Betty configuration */
	int indent_width;
	int use_tabs;
	int max_line_length;
} Formatter;

/* Formatter lifecycle */
Formatter *formatter_create(FILE *output);
void formatter_destroy(Formatter *formatter);

/* Main formatting */
int formatter_format(Formatter *formatter, ASTNode *ast);

#endif /* FORMATTER_H */
