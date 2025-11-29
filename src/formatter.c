#include "../include/formatter.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static void format_node(Formatter *fmt, ASTNode *node);
static void format_program(Formatter *fmt, ASTNode *node);
static void format_function(Formatter *fmt, ASTNode *node);
static void format_block(Formatter *fmt, ASTNode *node);
static void format_var_decl(Formatter *fmt, ASTNode *node);
static void format_if(Formatter *fmt, ASTNode *node);
static void format_while(Formatter *fmt, ASTNode *node);
static void format_for(Formatter *fmt, ASTNode *node);
static void format_do_while(Formatter *fmt, ASTNode *node);
static void format_switch(Formatter *fmt, ASTNode *node);
static void format_return(Formatter *fmt, ASTNode *node);
static void format_expression(Formatter *fmt, ASTNode *node);
static void format_binary(Formatter *fmt, ASTNode *node);
static void format_unary(Formatter *fmt, ASTNode *node);
static void format_call(Formatter *fmt, ASTNode *node);
static void format_struct(Formatter *fmt, ASTNode *node);
static void format_typedef(Formatter *fmt, ASTNode *node);
static void format_enum(Formatter *fmt, ASTNode *node);

/* Output helpers */
static void emit(Formatter *fmt, const char *str);
static void emit_char(Formatter *fmt, char c);
static void emit_newline(Formatter *fmt);
static void emit_indent(Formatter *fmt);
static void emit_space(Formatter *fmt);

/*
 * formatter_create - Create a new formatter
 * @output: Output file stream
 *
 * Return: Pointer to new formatter, or NULL on failure
 */
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

	/* Betty defaults */
	formatter->indent_width = 8;
	formatter->use_tabs = 1;
	formatter->max_line_length = 80;

	return (formatter);
}

/*
 * formatter_destroy - Free formatter memory
 * @formatter: Formatter to destroy
 */
void formatter_destroy(Formatter *formatter)
{
	if (!formatter)
		return;

	free(formatter);
}

/*
 * formatter_format - Format AST to output
 * @formatter: Formatter instance
 * @ast: Root AST node
 *
 * Return: 0 on success, -1 on error
 */
int formatter_format(Formatter *formatter, ASTNode *ast)
{
	if (!formatter || !ast)
		return (-1);

	format_node(formatter, ast);

	return (0);
}

/*
 * Output helpers
 */

static void emit(Formatter *fmt, const char *str)
{
	if (!str)
		return;

	while (*str)
	{
		emit_char(fmt, *str);
		str++;
	}
}

static void emit_char(Formatter *fmt, char c)
{
	fputc(c, fmt->output);

	if (c == '\n')
	{
		fmt->column = 0;
		fmt->line++;
		fmt->at_line_start = 1;
	}
	else
	{
		if (c == '\t')
			fmt->column += fmt->indent_width -
				(fmt->column % fmt->indent_width);
		else
			fmt->column++;
		fmt->at_line_start = 0;
	}
}

static void emit_newline(Formatter *fmt)
{
	emit_char(fmt, '\n');
}

static void emit_indent(Formatter *fmt)
{
	int i;

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

static void emit_space(Formatter *fmt)
{
	emit_char(fmt, ' ');
}

/*
 * Node formatting dispatch
 */

static void format_node(Formatter *fmt, ASTNode *node)
{
	if (!node)
		return;

	switch (node->type)
	{
	case NODE_PROGRAM:
		format_program(fmt, node);
		break;
	case NODE_FUNCTION:
		format_function(fmt, node);
		break;
	case NODE_BLOCK:
		format_block(fmt, node);
		break;
	case NODE_VAR_DECL:
		format_var_decl(fmt, node);
		break;
	case NODE_IF:
		format_if(fmt, node);
		break;
	case NODE_WHILE:
		format_while(fmt, node);
		break;
	case NODE_FOR:
		format_for(fmt, node);
		break;
	case NODE_DO_WHILE:
		format_do_while(fmt, node);
		break;
	case NODE_SWITCH:
		format_switch(fmt, node);
		break;
	case NODE_RETURN:
		format_return(fmt, node);
		break;
	case NODE_BREAK:
		emit_indent(fmt);
		emit(fmt, "break;");
		emit_newline(fmt);
		break;
	case NODE_CONTINUE:
		emit_indent(fmt);
		emit(fmt, "continue;");
		emit_newline(fmt);
		break;
	case NODE_EXPR_STMT:
		emit_indent(fmt);
		if (node->child_count > 0)
			format_expression(fmt, node->children[0]);
		emit(fmt, ";");
		emit_newline(fmt);
		break;
	case NODE_STRUCT:
		format_struct(fmt, node);
		break;
	case NODE_TYPEDEF:
		format_typedef(fmt, node);
		break;
	case NODE_ENUM:
		format_enum(fmt, node);
		break;
	case NODE_BINARY:
	case NODE_UNARY:
	case NODE_CALL:
	case NODE_LITERAL:
	case NODE_IDENTIFIER:
	case NODE_MEMBER_ACCESS:
	case NODE_ARRAY_ACCESS:
	case NODE_CAST:
	case NODE_SIZEOF:
	case NODE_TERNARY:
		format_expression(fmt, node);
		break;
	default:
		break;
	}
}

/*
 * Program formatting
 */

static void format_program(Formatter *fmt, ASTNode *node)
{
	int i;
	int prev_was_function = 0;

	for (i = 0; i < node->child_count; i++)
	{
		ASTNode *child = node->children[i];

		if (child->type == NODE_FUNCTION && prev_was_function)
			emit_newline(fmt);

		format_node(fmt, child);

		prev_was_function = (child->type == NODE_FUNCTION);
	}
}

/*
 * Function formatting - Betty style
 */

static void format_function(Formatter *fmt, ASTNode *node)
{
	Token *token = node->token;

	if (!token)
		return;

	emit(fmt, "/* function: ");
	emit(fmt, token->lexeme);
	emit(fmt, " */");
	emit_newline(fmt);

	if (node->child_count > 0)
	{
		emit(fmt, "{");
		emit_newline(fmt);
		fmt->indent_level++;

		if (node->children[0]->type == NODE_BLOCK)
		{
			ASTNode *block = node->children[0];
			int j;

			for (j = 0; j < block->child_count; j++)
				format_node(fmt, block->children[j]);
		}
		else
		{
			format_node(fmt, node->children[0]);
		}

		fmt->indent_level--;
		emit(fmt, "}");
		emit_newline(fmt);
	}
}

/*
 * Block formatting
 */

static void format_block(Formatter *fmt, ASTNode *node)
{
	int i;

	emit_newline(fmt);
	emit_indent(fmt);
	emit(fmt, "{");
	emit_newline(fmt);

	fmt->indent_level++;

	for (i = 0; i < node->child_count; i++)
		format_node(fmt, node->children[i]);

	fmt->indent_level--;

	emit_indent(fmt);
	emit(fmt, "}");
	emit_newline(fmt);
}

/*
 * Variable declaration formatting
 */

static void format_var_decl(Formatter *fmt, ASTNode *node)
{
	Token *type_token = node->token;
	Token *name_token = (Token *)node->data;

	emit_indent(fmt);

	if (type_token && type_token->lexeme)
		emit(fmt, type_token->lexeme);

	emit_space(fmt);

	if (name_token && name_token->lexeme)
		emit(fmt, name_token->lexeme);
	else
		emit(fmt, "var");

	if (node->child_count > 0)
	{
		emit(fmt, " = ");
		format_expression(fmt, node->children[0]);
	}

	emit(fmt, ";");
	emit_newline(fmt);
}

/*
 * If statement formatting - Betty style
 */

static void format_if(Formatter *fmt, ASTNode *node)
{
	emit_indent(fmt);
	emit(fmt, "if (");

	if (node->child_count > 0)
		format_expression(fmt, node->children[0]);

	emit(fmt, ")");

	if (node->child_count > 1)
	{
		ASTNode *then_branch = node->children[1];

		if (then_branch->type == NODE_BLOCK)
		{
			format_block(fmt, then_branch);
		}
		else
		{
			emit_newline(fmt);
			fmt->indent_level++;
			format_node(fmt, then_branch);
			fmt->indent_level--;
		}
	}

	if (node->child_count > 2)
	{
		ASTNode *else_branch = node->children[2];

		emit_indent(fmt);
		emit(fmt, "else");

		if (else_branch->type == NODE_IF)
		{
			emit_space(fmt);
			emit(fmt, "if (");
			if (else_branch->child_count > 0)
				format_expression(fmt, else_branch->children[0]);
			emit(fmt, ")");

			if (else_branch->child_count > 1)
			{
				if (else_branch->children[1]->type == NODE_BLOCK)
					format_block(fmt, else_branch->children[1]);
				else
				{
					emit_newline(fmt);
					fmt->indent_level++;
					format_node(fmt, else_branch->children[1]);
					fmt->indent_level--;
				}
			}
		}
		else if (else_branch->type == NODE_BLOCK)
		{
			format_block(fmt, else_branch);
		}
		else
		{
			emit_newline(fmt);
			fmt->indent_level++;
			format_node(fmt, else_branch);
			fmt->indent_level--;
		}
	}
}

/*
 * While statement formatting
 */

static void format_while(Formatter *fmt, ASTNode *node)
{
	emit_indent(fmt);
	emit(fmt, "while (");

	if (node->child_count > 0)
		format_expression(fmt, node->children[0]);

	emit(fmt, ")");

	if (node->child_count > 1)
	{
		if (node->children[1]->type == NODE_BLOCK)
			format_block(fmt, node->children[1]);
		else
		{
			emit_newline(fmt);
			fmt->indent_level++;
			format_node(fmt, node->children[1]);
			fmt->indent_level--;
		}
	}
}

/*
 * For statement formatting
 */

static void format_for(Formatter *fmt, ASTNode *node)
{
	emit_indent(fmt);
	emit(fmt, "for (");

	if (node->child_count > 0 && node->children[0])
		format_expression(fmt, node->children[0]);
	emit(fmt, "; ");

	if (node->child_count > 1 && node->children[1])
		format_expression(fmt, node->children[1]);
	emit(fmt, "; ");

	if (node->child_count > 2 && node->children[2])
		format_expression(fmt, node->children[2]);
	emit(fmt, ")");

	if (node->child_count > 3)
	{
		if (node->children[3]->type == NODE_BLOCK)
			format_block(fmt, node->children[3]);
		else
		{
			emit_newline(fmt);
			fmt->indent_level++;
			format_node(fmt, node->children[3]);
			fmt->indent_level--;
		}
	}
}

/*
 * Do-while statement formatting
 */

static void format_do_while(Formatter *fmt, ASTNode *node)
{
	emit_indent(fmt);
	emit(fmt, "do");

	if (node->child_count > 0)
	{
		if (node->children[0]->type == NODE_BLOCK)
			format_block(fmt, node->children[0]);
		else
		{
			emit_newline(fmt);
			fmt->indent_level++;
			format_node(fmt, node->children[0]);
			fmt->indent_level--;
		}
	}

	emit_indent(fmt);
	emit(fmt, "while (");

	if (node->child_count > 1)
		format_expression(fmt, node->children[1]);

	emit(fmt, ");");
	emit_newline(fmt);
}

/*
 * Switch statement formatting
 */

static void format_switch(Formatter *fmt, ASTNode *node)
{
	int i;

	emit_indent(fmt);
	emit(fmt, "switch (");

	if (node->child_count > 0)
		format_expression(fmt, node->children[0]);

	emit(fmt, ")");
	emit_newline(fmt);
	emit_indent(fmt);
	emit(fmt, "{");
	emit_newline(fmt);

	for (i = 1; i < node->child_count; i++)
	{
		ASTNode *case_node = node->children[i];

		if (case_node->type == NODE_CASE)
		{
			int stmt_start = 0;

			emit_indent(fmt);

			/* Check if it's 'case' or 'default' by token type */
			if (case_node->token &&
			    case_node->token->type == TOK_DEFAULT)
			{
				emit(fmt, "default:");
				stmt_start = 0;
			}
			else
			{
				emit(fmt, "case ");
				/* Case value is first child */
				if (case_node->child_count > 0)
				{
					format_expression(fmt, case_node->children[0]);
					stmt_start = 1;
				}
				emit(fmt, ":");
			}
			emit_newline(fmt);

			/* Format case body statements */
			fmt->indent_level++;
			{
				int j;

				for (j = stmt_start; j < case_node->child_count; j++)
					format_node(fmt, case_node->children[j]);
			}
			fmt->indent_level--;
		}
	}

	emit_indent(fmt);
	emit(fmt, "}");
	emit_newline(fmt);
}

/*
 * Return statement formatting - Betty requires parentheses
 */

static void format_return(Formatter *fmt, ASTNode *node)
{
	emit_indent(fmt);
	emit(fmt, "return");

	if (node->child_count > 0)
	{
		emit(fmt, " (");
		format_expression(fmt, node->children[0]);
		emit(fmt, ")");
	}

	emit(fmt, ";");
	emit_newline(fmt);
}

/*
 * Expression formatting
 */

static void format_expression(Formatter *fmt, ASTNode *node)
{
	if (!node)
		return;

	switch (node->type)
	{
	case NODE_LITERAL:
		if (node->token && node->token->lexeme)
			emit(fmt, node->token->lexeme);
		break;

	case NODE_IDENTIFIER:
		if (node->token && node->token->lexeme)
			emit(fmt, node->token->lexeme);
		break;

	case NODE_BINARY:
		format_binary(fmt, node);
		break;

	case NODE_UNARY:
		format_unary(fmt, node);
		break;

	case NODE_CALL:
		format_call(fmt, node);
		break;

	case NODE_MEMBER_ACCESS:
		if (node->child_count > 0)
			format_expression(fmt, node->children[0]);
		if (node->token && node->token->lexeme)
		{
			emit(fmt, "->");
			emit(fmt, node->token->lexeme);
		}
		break;

	case NODE_ARRAY_ACCESS:
		if (node->child_count > 0)
			format_expression(fmt, node->children[0]);
		emit(fmt, "[");
		if (node->child_count > 1)
			format_expression(fmt, node->children[1]);
		emit(fmt, "]");
		break;

	case NODE_CAST:
		emit(fmt, "(");
		if (node->token && node->token->lexeme)
			emit(fmt, node->token->lexeme);
		emit(fmt, ")");
		if (node->child_count > 0)
			format_expression(fmt, node->children[0]);
		break;

	case NODE_SIZEOF:
		emit(fmt, "sizeof(");
		if (node->child_count > 0)
		{
			/* sizeof(expression) */
			format_expression(fmt, node->children[0]);
		}
		else if (node->data)
		{
			/* sizeof(type) - type stored in data field */
			Token *type_token = (Token *)node->data;

			if (type_token && type_token->lexeme)
				emit(fmt, type_token->lexeme);
		}
		emit(fmt, ")");
		break;

	case NODE_TERNARY:
		if (node->child_count > 0)
			format_expression(fmt, node->children[0]);
		emit(fmt, " ? ");
		if (node->child_count > 1)
			format_expression(fmt, node->children[1]);
		emit(fmt, " : ");
		if (node->child_count > 2)
			format_expression(fmt, node->children[2]);
		break;

	default:
		break;
	}
}

/*
 * Binary expression formatting
 */

static void format_binary(Formatter *fmt, ASTNode *node)
{
	const char *op = "";

	if (node->token && node->token->lexeme)
		op = node->token->lexeme;

	if (node->child_count > 0)
		format_expression(fmt, node->children[0]);

	emit_space(fmt);
	emit(fmt, op);
	emit_space(fmt);

	if (node->child_count > 1)
		format_expression(fmt, node->children[1]);
}

/*
 * Unary expression formatting
 */

static void format_unary(Formatter *fmt, ASTNode *node)
{
	const char *op = "";

	if (node->token && node->token->lexeme)
		op = node->token->lexeme;

	emit(fmt, op);
	if (node->child_count > 0)
		format_expression(fmt, node->children[0]);
}

/*
 * Function call formatting
 */

static void format_call(Formatter *fmt, ASTNode *node)
{
	int i;
	int arg_start = 0;

	if (node->token && node->token->lexeme)
	{
		emit(fmt, node->token->lexeme);
	}
	else if (node->child_count > 0)
	{
		format_expression(fmt, node->children[0]);
		arg_start = 1;
	}

	emit(fmt, "(");

	for (i = arg_start; i < node->child_count; i++)
	{
		if (i > arg_start)
			emit(fmt, ", ");
		format_expression(fmt, node->children[i]);
	}

	emit(fmt, ")");
}

/*
 * Struct formatting
 */

static void format_struct(Formatter *fmt, ASTNode *node)
{
	emit(fmt, "struct");

	if (node->token && node->token->lexeme)
	{
		emit_space(fmt);
		emit(fmt, node->token->lexeme);
	}

	emit_newline(fmt);
}

/*
 * Typedef formatting
 */

static void format_typedef(Formatter *fmt, ASTNode *node)
{
	emit(fmt, "typedef ");

	if (node->child_count > 0)
		format_node(fmt, node->children[0]);

	if (node->token && node->token->lexeme)
	{
		emit_space(fmt);
		emit(fmt, node->token->lexeme);
	}

	emit(fmt, ";");
	emit_newline(fmt);
}

/*
 * Enum formatting
 */

static void format_enum(Formatter *fmt, ASTNode *node)
{
	emit(fmt, "enum");

	if (node->token && node->token->lexeme)
	{
		emit_space(fmt);
		emit(fmt, node->token->lexeme);
	}

	emit_newline(fmt);
}