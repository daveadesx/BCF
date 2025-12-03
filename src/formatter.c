#include "../include/formatter.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static void format_node(Formatter *fmt, ASTNode *node);
static void format_program(Formatter *fmt, ASTNode *node);
static void format_function(Formatter *fmt, ASTNode *node);
static void format_block(Formatter *fmt, ASTNode *node);
static void format_var_decl(Formatter *fmt, ASTNode *node);
static void format_func_ptr(Formatter *fmt, ASTNode *node);
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
 * format_comment - Format a single comment, converting C99 to C89 style
 * @fmt: Formatter instance
 * @comment: Comment token
 * @inline_comment: 1 if this is an inline comment (same line as code)
 */
static void format_comment(Formatter *fmt, Token *comment, int inline_comment)
{
	const char *text;
	int len;

	if (!comment || !comment->lexeme)
		return;

	text = comment->lexeme;
	len = strlen(text);

	if (!inline_comment && !fmt->at_line_start)
		emit_newline(fmt);

	if (!inline_comment)
		emit_indent(fmt);
	else
		emit(fmt, " ");

	/* Check if C99 style comment (// ...) */
	if (len >= 2 && text[0] == '/' && text[1] == '/')
	{
		/* Convert C99 to C89 style */
		emit(fmt, "/*");
		emit(fmt, text + 2);  /* Skip the // */
		emit(fmt, " */");
	}
	else
	{
		/* Already C89 block comment */
		emit(fmt, text);
	}

	if (!inline_comment)
		emit_newline(fmt);
}

/*
 * emit_leading_comments - Output all leading comments for a node
 * @fmt: Formatter instance
 * @node: Node whose leading comments to output
 */
static void emit_leading_comments(Formatter *fmt, ASTNode *node)
{
	int i;

	if (!node || node->leading_comment_count == 0)
		return;

	for (i = 0; i < node->leading_comment_count; i++)
		format_comment(fmt, node->leading_comments[i], 0);
}

/*
 * emit_trailing_comments - Output all trailing comments for a node
 * @fmt: Formatter instance
 * @node: Node whose trailing comments to output
 *
 * Trailing comments appear on the same line, after the code.
 */
static void emit_trailing_comments(Formatter *fmt, ASTNode *node)
{
	int i;

	if (!node || node->trailing_comment_count == 0)
		return;

	for (i = 0; i < node->trailing_comment_count; i++)
		format_comment(fmt, node->trailing_comments[i], 1);
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
	case NODE_FUNC_PTR:
		format_func_ptr(fmt, node);
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
		emit_trailing_comments(fmt, node);
		emit_newline(fmt);
		break;
	case NODE_CONTINUE:
		emit_indent(fmt);
		emit(fmt, "continue;");
		emit_trailing_comments(fmt, node);
		emit_newline(fmt);
		break;
	case NODE_EXPR_STMT:
		emit_indent(fmt);
		if (node->child_count > 0)
			format_expression(fmt, node->children[0]);
		emit(fmt, ";");
		emit_trailing_comments(fmt, node);
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
	case NODE_PREPROCESSOR:
		/* Output preprocessor directive verbatim */
		if (node->token && node->token->lexeme)
		{
			emit(fmt, node->token->lexeme);
			emit_newline(fmt);
		}
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
	case NODE_TYPE_EXPR:
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
	NodeType prev_type = NODE_PROGRAM;  /* Initial sentinel */
	ASTNode *prev_child = NULL;

	for (i = 0; i < node->child_count; i++)
	{
		ASTNode *child = node->children[i];
		int need_blank = 0;
		int prev_is_conditional_start = 0;
		int curr_is_conditional_end = 0;

		/* Check if previous was a conditional compilation start */
		if (prev_child && prev_type == NODE_PREPROCESSOR &&
		    prev_child->token && prev_child->token->lexeme)
		{
			const char *lex = prev_child->token->lexeme;
			if (strncmp(lex, "#ifdef", 6) == 0 ||
			    strncmp(lex, "#ifndef", 7) == 0 ||
			    strncmp(lex, "#if ", 4) == 0 ||
			    strncmp(lex, "#if\t", 4) == 0 ||
			    strncmp(lex, "#else", 5) == 0 ||
			    strncmp(lex, "#elif", 5) == 0)
				prev_is_conditional_start = 1;
		}

		/* Check if current is a conditional compilation end/else */
		if (child->type == NODE_PREPROCESSOR &&
		    child->token && child->token->lexeme)
		{
			const char *lex = child->token->lexeme;
			if (strncmp(lex, "#endif", 6) == 0 ||
			    strncmp(lex, "#else", 5) == 0 ||
			    strncmp(lex, "#elif", 5) == 0)
				curr_is_conditional_end = 1;
		}

		/* Add blank lines for readability */
		if (i > 0)
		{
			/* No blank line between consecutive preprocessor directives */
			if (prev_type == NODE_PREPROCESSOR &&
			    child->type == NODE_PREPROCESSOR)
				need_blank = 0;
			/* No blank line after #ifdef/#if/#else before code */
			else if (prev_is_conditional_start)
				need_blank = 0;
			/* No blank line before #endif/#else/#elif after code */
			else if (curr_is_conditional_end)
				need_blank = 0;
			/* Blank line after preprocessor block before code */
			else if (prev_type == NODE_PREPROCESSOR &&
				 child->type != NODE_PREPROCESSOR)
				need_blank = 1;
			/* Blank line before preprocessor if after code */
			else if (child->type == NODE_PREPROCESSOR &&
				 prev_type != NODE_PREPROCESSOR &&
				 prev_type != NODE_PROGRAM)
				need_blank = 1;
			/* Blank line after functions */
			else if (prev_type == NODE_FUNCTION)
				need_blank = 1;
			/* Blank line after struct/enum/typedef definitions */
			else if (prev_type == NODE_STRUCT || prev_type == NODE_ENUM ||
				 prev_type == NODE_TYPEDEF)
				need_blank = 1;
			/* Blank line after global variable declarations */
			else if (prev_type == NODE_VAR_DECL || prev_type == NODE_FUNC_PTR)
				need_blank = 1;
			/* Blank line before a function if anything is above it */
			else if (child->type == NODE_FUNCTION)
				need_blank = 1;
			/* Blank line before typedef/struct/enum if anything is above */
			else if (child->type == NODE_TYPEDEF ||
				 child->type == NODE_STRUCT ||
				 child->type == NODE_ENUM)
				need_blank = 1;
			/* Preserve user-added blank line */
			else if (child->blank_lines_before > 0)
				need_blank = 1;
		}

		if (need_blank)
			emit_newline(fmt);

		/* Output leading comments */
		emit_leading_comments(fmt, child);

		format_node(fmt, child);

		/* Add semicolon and newline for standalone struct/enum declarations */
		if (child->type == NODE_STRUCT || child->type == NODE_ENUM)
		{
			emit(fmt, ";");
			emit_newline(fmt);
		}

		prev_type = child->type;
		prev_child = child;
	}
}

/*
 * Function formatting - Betty style
 */

static void format_function(Formatter *fmt, ASTNode *node)
{
	Token *name_token = node->token;
	FunctionData *func_data = (FunctionData *)node->data;
	int i;

	if (!name_token)
		return;

	/* Output return type */
	if (func_data && func_data->return_type_count > 0)
	{
		int last_was_star = 0;

		for (i = 0; i < func_data->return_type_count; i++)
		{
			Token *tok = func_data->return_type_tokens[i];

			if (tok->type == TOK_STAR)
			{
				/* Pointer - no space before, but check if previous was * */
				if (i > 0 && !last_was_star)
					emit(fmt, " ");
				emit(fmt, "*");
				last_was_star = 1;
			}
			else
			{
				if (i > 0)
					emit(fmt, " ");
				emit(fmt, tok->lexeme);
				last_was_star = 0;
			}
		}
		/* Only add space if last token wasn't a pointer */
		if (!last_was_star)
			emit(fmt, " ");
	}

	/* Function name (same line as return type) */
	emit(fmt, name_token->lexeme);
	emit(fmt, "(");

	/* Output parameters */
	if (func_data && func_data->param_count > 0)
	{
		for (i = 0; i < func_data->param_count; i++)
		{
			ASTNode *param = func_data->params[i];
			FunctionData *pdata = (FunctionData *)param->data;
			int j;
			int bracket_start = -1;
			int last_was_star = 0;

			if (i > 0)
				emit(fmt, ", ");

			/* Handle variadic parameter (...) */
			if (param->token && param->token->type == TOK_ELLIPSIS)
			{
				emit(fmt, "...");
				continue;
			}

			/* Output parameter type (but not brackets) */
			if (pdata && pdata->return_type_count > 0)
			{
				for (j = 0; j < pdata->return_type_count; j++)
				{
					Token *tok = pdata->return_type_tokens[j];

					if (tok->type == TOK_STAR)
					{
						if (j > 0 && !last_was_star)
							emit(fmt, " ");
						emit(fmt, "*");
						last_was_star = 1;
					}
					else if (tok->type == TOK_LBRACKET)
					{
						bracket_start = j;
						break;  /* Stop here, output brackets after name */
					}
					else
					{
						/* Add space before keyword only if not after * */
						if (j > 0 && !last_was_star)
							emit(fmt, " ");
						emit(fmt, tok->lexeme);
						last_was_star = 0;
					}
				}
			}

			/* Output parameter name */
			if (param->token)
			{
				/* No space after pointer, but space after type keyword */
				if (pdata && pdata->return_type_count > 0 &&
				    bracket_start != 0 && !last_was_star)
					emit(fmt, " ");
				emit(fmt, param->token->lexeme);
			}

			/* Output array brackets after name */
			if (bracket_start >= 0 && pdata)
			{
				for (j = bracket_start; j < pdata->return_type_count; j++)
				{
					Token *tok = pdata->return_type_tokens[j];

					if (tok->type == TOK_LBRACKET)
						emit(fmt, "[");
					else if (tok->type == TOK_RBRACKET)
						emit(fmt, "]");
				}
			}
		}
	}
	else
	{
		emit(fmt, "void");
	}

	emit(fmt, ")");

	/* Function body or prototype */
	if (node->child_count > 0)
	{
		emit_newline(fmt);
		emit(fmt, "{");
		emit_newline(fmt);
		fmt->indent_level++;

		if (node->children[0]->type == NODE_BLOCK)
		{
			ASTNode *block = node->children[0];
			int j;
			int had_var_decl = 0;
			int added_blank = 0;

			for (j = 0; j < block->child_count; j++)
			{
				ASTNode *stmt = block->children[j];
				int is_var_decl = (stmt->type == NODE_VAR_DECL ||
						   stmt->type == NODE_FUNC_PTR);
				int need_blank = 0;

				/* Add blank line when transitioning from decls to stmts */
				if (had_var_decl && !is_var_decl && !added_blank)
				{
					need_blank = 1;
					added_blank = 1;
				}
				/* Preserve user-added blank lines (after first decl->stmt transition) */
				else if (added_blank && stmt->blank_lines_before > 0)
				{
					need_blank = 1;
				}

				if (need_blank)
					emit_newline(fmt);

				/* Output leading comments for this statement */
				emit_leading_comments(fmt, stmt);

				if (is_var_decl)
					had_var_decl = 1;

				format_node(fmt, stmt);
			}
		}
		else
		{
			format_node(fmt, node->children[0]);
		}

		fmt->indent_level--;
		emit(fmt, "}");
		emit_newline(fmt);
	}
	else
	{
		/* Function prototype - just semicolon */
		emit(fmt, ";");
		emit_newline(fmt);
	}
}

/*
 * Block formatting
 */

static void format_block(Formatter *fmt, ASTNode *node)
{
	int i;
	int had_var_decl = 0;
	int added_blank = 0;

	emit_newline(fmt);
	emit_indent(fmt);
	emit(fmt, "{");
	emit_newline(fmt);

	fmt->indent_level++;

	for (i = 0; i < node->child_count; i++)
	{
		ASTNode *stmt = node->children[i];
		int is_var_decl = (stmt->type == NODE_VAR_DECL ||
				   stmt->type == NODE_FUNC_PTR);
		int need_blank = 0;

		/* Add blank line when transitioning from decls to stmts */
		if (had_var_decl && !is_var_decl && !added_blank)
		{
			need_blank = 1;
			added_blank = 1;
		}
		/* Preserve user-added blank lines */
		else if (added_blank && stmt->blank_lines_before > 0)
		{
			need_blank = 1;
		}

		if (need_blank)
			emit_newline(fmt);

		/* Output leading comments for this statement */
		emit_leading_comments(fmt, stmt);

		if (is_var_decl)
			had_var_decl = 1;

		format_node(fmt, stmt);
	}

	fmt->indent_level--;

	emit_indent(fmt);
	emit(fmt, "}");
	emit_newline(fmt);
}

/*
 * Variable declaration formatting
 */

/*
 * Helper to output a single variable declaration
 */
static void format_single_var(Formatter *fmt, VarDeclData *var_data)
{
	int i;
	int last_was_star = 0;

	if (!var_data || var_data->type_count == 0)
		return;

	/* Output type tokens */
	for (i = 0; i < var_data->type_count; i++)
	{
		Token *tok = var_data->type_tokens[i];

		if (tok->type == TOK_STAR)
		{
			if (i > 0 && !last_was_star)
				emit(fmt, " ");
			emit(fmt, "*");
			last_was_star = 1;
		}
		else
		{
			if (i > 0 && !last_was_star)
				emit(fmt, " ");
			emit(fmt, tok->lexeme);
			last_was_star = 0;
		}
	}

	/* Output variable name */
	if (var_data->name_token)
	{
		if (!last_was_star)
			emit(fmt, " ");
		emit(fmt, var_data->name_token->lexeme);
	}

	/* Output array brackets */
	if (var_data->array_count > 0)
	{
		for (i = 0; i < var_data->array_count; i++)
		{
			Token *tok = var_data->array_tokens[i];

			if (tok->type == TOK_LBRACKET)
				emit(fmt, "[");
			else if (tok->type == TOK_RBRACKET)
				emit(fmt, "]");
			else
				emit(fmt, tok->lexeme);
		}
	}

	/* Output initialization if present */
	if (var_data->init_expr)
	{
		emit(fmt, " = ");
		format_expression(fmt, var_data->init_expr);
	}
}

/*
 * Helper to output just name, array, and init (no type) for comma-separated vars
 */
static void format_extra_var(Formatter *fmt, VarDeclData *var_data)
{
	int i;
	int has_ptr = 0;

	if (!var_data)
		return;

	/* Check if this var has pointers */
	for (i = 0; i < var_data->type_count; i++)
	{
		if (var_data->type_tokens[i]->type == TOK_STAR)
		{
			emit(fmt, "*");
			has_ptr = 1;
		}
	}

	/* Output variable name */
	if (var_data->name_token)
		emit(fmt, var_data->name_token->lexeme);

	/* Output array brackets */
	if (var_data->array_count > 0)
	{
		for (i = 0; i < var_data->array_count; i++)
		{
			Token *tok = var_data->array_tokens[i];

			if (tok->type == TOK_LBRACKET)
				emit(fmt, "[");
			else if (tok->type == TOK_RBRACKET)
				emit(fmt, "]");
			else
				emit(fmt, tok->lexeme);
		}
	}

	/* Output initialization if present */
	if (var_data->init_expr)
	{
		emit(fmt, " = ");
		format_expression(fmt, var_data->init_expr);
	}

	(void)has_ptr;
}

static void format_var_decl(Formatter *fmt, ASTNode *node)
{
	VarDeclData *var_data = (VarDeclData *)node->data;
	int i;

	emit_indent(fmt);

	if (var_data && var_data->type_count > 0)
	{
		format_single_var(fmt, var_data);

		/* Output extra variables on same line with commas */
		if (var_data->extra_count > 0)
		{
			for (i = 0; i < var_data->extra_count; i++)
			{
				emit(fmt, ", ");
				format_extra_var(fmt, var_data->extra_vars[i]);
			}
		}
	}
	else
	{
		/* Fallback for old-style nodes */
		Token *type_token = node->token;

		if (type_token && type_token->lexeme)
			emit(fmt, type_token->lexeme);
		emit_space(fmt);
		emit(fmt, "var");

		if (node->child_count > 0)
		{
			emit(fmt, " = ");
			format_expression(fmt, node->children[0]);
		}
	}

	emit(fmt, ";");
	emit_trailing_comments(fmt, node);
	emit_newline(fmt);
}

/*
 * Function pointer formatting - Betty style
 * Output: return_type (*name)(params);
 */
static void emit_func_ptr_content(Formatter *fmt, FuncPtrData *fp_data)
{
	int i;
	int need_space = 0;
	int ends_with_ptr = 0;

	/* Output return type tokens */
	for (i = 0; i < fp_data->return_type_count; i++)
	{
		Token *tok = fp_data->return_type_tokens[i];

		if (tok->type == TOK_STAR)
		{
			emit(fmt, " *");
			need_space = 0;
			ends_with_ptr = 1;
		}
		else
		{
			if (need_space)
				emit_space(fmt);
			emit(fmt, tok->lexeme);
			need_space = 1;
			ends_with_ptr = 0;
		}
	}

	/* Emit (*name) - add space if not ending with pointer */
	if (!ends_with_ptr)
		emit_space(fmt);
	emit(fmt, "(*");
	emit(fmt, fp_data->name_token->lexeme);
	emit(fmt, ")(");

	/* Output parameter tokens */
	need_space = 0;
	for (i = 0; i < fp_data->param_count; i++)
	{
		Token *tok = fp_data->param_tokens[i];

		if (tok->type == TOK_COMMA)
		{
			emit(fmt, ",");
			need_space = 1;
		}
		else if (tok->type == TOK_STAR)
		{
			if (need_space)
				emit_space(fmt);
			emit(fmt, "*");
			need_space = 0;
		}
		else
		{
			if (need_space)
				emit_space(fmt);
			emit(fmt, tok->lexeme);
			need_space = 1;
		}
	}

	emit(fmt, ")");
}

static void format_func_ptr(Formatter *fmt, ASTNode *node)
{
	FuncPtrData *fp_data = (FuncPtrData *)node->data;

	emit_indent(fmt);

	if (fp_data)
		emit_func_ptr_content(fmt, fp_data);

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
			/* Handle else if by removing indent and formatting as "else if" */
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

			/* Recursively handle nested else/else-if */
			if (else_branch->child_count > 2)
			{
				ASTNode *nested_else = else_branch->children[2];

				emit_indent(fmt);
				emit(fmt, "else");

				if (nested_else->type == NODE_IF)
				{
					/* Recursive call for else-if chains */
					emit_space(fmt);
					/* Re-use format_if but skip the "if" part */
					emit(fmt, "if (");
					if (nested_else->child_count > 0)
						format_expression(fmt, nested_else->children[0]);
					emit(fmt, ")");
					if (nested_else->child_count > 1)
					{
						if (nested_else->children[1]->type == NODE_BLOCK)
							format_block(fmt, nested_else->children[1]);
						else
						{
							emit_newline(fmt);
							fmt->indent_level++;
							format_node(fmt, nested_else->children[1]);
							fmt->indent_level--;
						}
					}
					/* Handle deeper nesting if needed */
					if (nested_else->child_count > 2)
					{
						/* Just format the else branch directly */
						emit_indent(fmt);
						emit(fmt, "else");
						if (nested_else->children[2]->type == NODE_BLOCK)
							format_block(fmt, nested_else->children[2]);
						else
						{
							emit_newline(fmt);
							fmt->indent_level++;
							format_node(fmt, nested_else->children[2]);
							fmt->indent_level--;
						}
					}
				}
				else if (nested_else->type == NODE_BLOCK)
				{
					format_block(fmt, nested_else);
				}
				else
				{
					emit_newline(fmt);
					fmt->indent_level++;
					format_node(fmt, nested_else);
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
	emit_trailing_comments(fmt, node);
	emit_newline(fmt);
}

/*
 * Expression formatting
 */

static void format_expression(Formatter *fmt, ASTNode *node)
{
	int i;

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

	case NODE_INIT_LIST:
		emit(fmt, "{");
		for (i = 0; i < node->child_count; i++)
		{
			if (i > 0)
				emit(fmt, ", ");
			format_expression(fmt, node->children[i]);
		}
		emit(fmt, "}");
		break;

	case NODE_TYPE_EXPR:
		/* Type used as expression (e.g., va_arg second argument) */
		if (node->data)
		{
			FunctionData *type_data = (FunctionData *)node->data;
			int j;
			int last_was_star = 0;

			for (j = 0; j < type_data->return_type_count; j++)
			{
				Token *tok = type_data->return_type_tokens[j];

				if (tok->type == TOK_STAR)
				{
					if (j > 0 && !last_was_star)
						emit(fmt, " ");
					emit(fmt, "*");
					last_was_star = 1;
				}
				else
				{
					if (j > 0 && !last_was_star)
						emit(fmt, " ");
					emit(fmt, tok->lexeme);
					last_was_star = 0;
				}
			}
		}
		else if (node->token && node->token->lexeme)
		{
			emit(fmt, node->token->lexeme);
		}
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
	int i;

	emit(fmt, "struct");

	if (node->token && node->token->lexeme)
	{
		emit_space(fmt);
		emit(fmt, node->token->lexeme);
	}

	/* If struct has members (body), format them */
	if (node->child_count > 0)
	{
		emit_newline(fmt);
		emit(fmt, "{");
		emit_newline(fmt);
		fmt->indent_level++;

		for (i = 0; i < node->child_count; i++)
		{
			format_node(fmt, node->children[i]);
		}

		fmt->indent_level--;
		emit_indent(fmt);
		emit(fmt, "}");
	}
}

/*
 * Typedef formatting
 */

static void format_typedef(Formatter *fmt, ASTNode *node)
{
	int i;
	int has_ptr = 0;
	TypedefData *td_data = (TypedefData *)node->data;

	emit(fmt, "typedef ");

	/* If has function pointer child, format it inline */
	if (node->child_count > 0 && node->children[0]->type == NODE_FUNC_PTR)
	{
		FuncPtrData *fp_data = (FuncPtrData *)node->children[0]->data;

		if (fp_data)
			emit_func_ptr_content(fmt, fp_data);
		emit(fmt, ";");
		emit_newline(fmt);
		return;
	}
	/* If has struct/enum child, format it */
	else if (node->child_count > 0)
		format_node(fmt, node->children[0]);
	else if (td_data && td_data->base_type_count > 0)
	{
		/* Check if we have a pointer */
		for (i = 0; i < td_data->base_type_count; i++)
		{
			if (td_data->base_type_tokens[i]->type == TOK_STAR)
				has_ptr = 1;
		}

		/* Output base type tokens for simple typedef */
		for (i = 0; i < td_data->base_type_count; i++)
		{
			Token *tok = td_data->base_type_tokens[i];

			if (tok->type == TOK_STAR)
			{
				emit(fmt, " *");
			}
			else if (tok->lexeme)
			{
				if (i > 0)
					emit_space(fmt);
				emit(fmt, tok->lexeme);
			}
		}
	}

	if (node->token && node->token->lexeme)
	{
		/* Add space before alias unless we just emitted a pointer */
		if (!has_ptr)
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
	int i;

	emit(fmt, "enum");

	if (node->token && node->token->lexeme)
	{
		emit_space(fmt);
		emit(fmt, node->token->lexeme);
	}

	/* If enum has values, format them */
	if (node->child_count > 0)
	{
		emit_newline(fmt);
		emit(fmt, "{");
		emit_newline(fmt);
		fmt->indent_level++;

		for (i = 0; i < node->child_count; i++)
		{
			emit_indent(fmt);
			/* Emit enum value name */
			if (node->children[i]->token && node->children[i]->token->lexeme)
			{
				emit(fmt, node->children[i]->token->lexeme);
			}

			/* If it has an initializer value */
			if (node->children[i]->child_count > 0 && 
			    node->children[i]->children[0]->token &&
			    node->children[i]->children[0]->token->lexeme)
			{
				emit(fmt, " = ");
				emit(fmt, node->children[i]->children[0]->token->lexeme);
			}

			/* Add comma except for last element */
			if (i < node->child_count - 1)
				emit(fmt, ",");
			emit_newline(fmt);
		}

		fmt->indent_level--;
		emit_indent(fmt);
		emit(fmt, "}");
	}
}