#include "../include/formatter.h"
#include <stdlib.h>
#include <string.h>

static void emit_char(Formatter *fmt, char c)
{
if (!fmt || !fmt->output)
return;

fputc(c, fmt->output);
if (c == '\n')
{
fmt->at_line_start = 1;
fmt->column = 0;
fmt->line++;
}
else
{
fmt->at_line_start = 0;
if (c == '\t')
fmt->column += fmt->indent_width - (fmt->column % fmt->indent_width);
else
fmt->column++;
}
}

static void emit_string(Formatter *fmt, const char *str)
{
if (!fmt || !fmt->output || !str)
return;

while (*str)
{
emit_char(fmt, *str);
str++;
}
}

static void emit_newline(Formatter *fmt)
{
emit_char(fmt, '\n');
}

static void emit_indent(Formatter *fmt)
{
int i;

if (!fmt)
return;

for (i = 0; i < fmt->indent_level; i++)
{
if (fmt->use_tabs)
emit_char(fmt, '\t');
else
{
int j;

for (j = 0; j < fmt->indent_width; j++)
emit_char(fmt, ' ');
}
}
}

static int is_keyword(TokenType type)
{
return (type >= TOK_IF && type <= TOK_REGISTER);
}

static int is_identifier_like(TokenType type)
{
return (type == TOK_IDENTIFIER || type == TOK_INTEGER ||
 type == TOK_FLOAT || type == TOK_STRING ||
 type == TOK_CHAR || is_keyword(type));
}

static int is_operator_token(TokenType type)
{
return (type >= TOK_PLUS && type <= TOK_COLON);
}

static int needs_space_between(TokenType prev, TokenType curr)
{
if (prev == TOK_EOF || prev == TOK_PREPROCESSOR)
return (0);

if (prev == TOK_LPAREN || prev == TOK_LBRACKET ||
    prev == TOK_DOT || prev == TOK_ARROW)
return (0);

if (curr == TOK_RPAREN || curr == TOK_RBRACKET ||
    curr == TOK_COMMA || curr == TOK_SEMICOLON ||
    curr == TOK_COLON || curr == TOK_DOT ||
    curr == TOK_ARROW)
return (0);

if (curr == TOK_LPAREN && is_keyword(prev))
return (1);

if (is_identifier_like(prev) && is_identifier_like(curr))
return (1);

if (is_operator_token(prev) || is_operator_token(curr))
return (1);

return (0);
}

static void ensure_leading_space(Formatter *fmt, TokenType prev, TokenType curr,
int saw_space)
{
if (!fmt)
return;

if (fmt->at_line_start)
{
emit_indent(fmt);
return;
}

if (saw_space || needs_space_between(prev, curr))
emit_char(fmt, ' ');
}

static void flush_preprocessor_line(Formatter *fmt, const char *text)
{
if (!fmt || !text)
return;

if (!fmt->at_line_start)
emit_newline(fmt);

emit_string(fmt, text);
emit_newline(fmt);
}

Formatter *formatter_create(FILE *output)
{
Formatter *formatter;

if (!output)
return (NULL);

formatter = malloc(sizeof(Formatter));
if (!formatter)
return (NULL);

formatter->output = output;
formatter->indent_level = 0;
formatter->column = 0;
formatter->line = 1;
formatter->at_line_start = 1;

formatter->indent_width = 8;
formatter->use_tabs = 1;
formatter->max_line_length = 80;

return (formatter);
}

void formatter_destroy(Formatter *formatter)
{
if (!formatter)
return;

free(formatter);
}

int formatter_format(Formatter *formatter, Token **tokens, int token_count)
{
TokenType prev_sig = TOK_EOF;
int saw_space = 0;
int paren_depth = 0;
int i;

if (!formatter || !tokens)
return (-1);

for (i = 0; i < token_count; i++)
{
Token *tok = tokens[i];

if (!tok || !tok->lexeme)
continue;

switch (tok->type)
{
case TOK_WHITESPACE:
if (!formatter->at_line_start)
saw_space = 1;
break;
case TOK_NEWLINE:
emit_newline(formatter);
formatter->at_line_start = 1;
saw_space = 0;
prev_sig = TOK_EOF;
break;
case TOK_PREPROCESSOR:
flush_preprocessor_line(formatter, tok->lexeme);
formatter->at_line_start = 1;
saw_space = 0;
prev_sig = TOK_PREPROCESSOR;
break;
case TOK_COMMENT_LINE:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_string(formatter, tok->lexeme);
emit_newline(formatter);
formatter->at_line_start = 1;
prev_sig = TOK_EOF;
break;
case TOK_COMMENT_BLOCK:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_string(formatter, tok->lexeme);
saw_space = 1;
prev_sig = TOK_COMMENT_BLOCK;
break;
case TOK_LBRACE:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, '{');
emit_newline(formatter);
formatter->indent_level++;
formatter->at_line_start = 1;
prev_sig = TOK_LBRACE;
break;
case TOK_RBRACE:
if (formatter->indent_level > 0)
formatter->indent_level--;
if (!formatter->at_line_start)
emit_newline(formatter);
emit_indent(formatter);
emit_char(formatter, '}');
emit_newline(formatter);
formatter->at_line_start = 1;
saw_space = 0;
prev_sig = TOK_RBRACE;
break;
case TOK_LPAREN:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, '(');
paren_depth++;
prev_sig = TOK_LPAREN;
break;
case TOK_RPAREN:
if (paren_depth > 0)
paren_depth--;
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, ')');
prev_sig = TOK_RPAREN;
break;
case TOK_LBRACKET:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, '[');
prev_sig = TOK_LBRACKET;
break;
case TOK_RBRACKET:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, ']');
prev_sig = TOK_RBRACKET;
break;
case TOK_SEMICOLON:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, ';');
if (paren_depth == 0)
{
emit_newline(formatter);
formatter->at_line_start = 1;
prev_sig = TOK_EOF;
}
else
{
saw_space = 1;
prev_sig = TOK_SEMICOLON;
}
break;
case TOK_COMMA:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
saw_space = 0;
emit_char(formatter, ',');
saw_space = 1;
prev_sig = TOK_COMMA;
break;
default:
ensure_leading_space(formatter, prev_sig, tok->type, saw_space);
emit_string(formatter, tok->lexeme);
saw_space = 0;
prev_sig = tok->type;
if (tok->type == TOK_ARROW || tok->type == TOK_DOT)
saw_space = 0;
else if (is_operator_token(tok->type) || is_identifier_like(tok->type))
saw_space = 1;
break;
}
}

if (!formatter->at_line_start)
emit_newline(formatter);

return (0);
}
