#ifndef FORMATTER_H
#define FORMATTER_H

#include <stdio.h>

/*
 * Simple C code formatter
 * Rules:
 * 1. Blank line after function/struct/variable declarations
 * 2. Blank line between different preprocessor directive groups
 * 3. Convert spaces to tabs (8-space tabs)
 * 4. Remove trailing whitespace
 * 5. Braces on new lines (except do-while opening brace)
 */

/* Format source code and write to output */
int format_source(const char *source, FILE *output);

#endif /* FORMATTER_H */
