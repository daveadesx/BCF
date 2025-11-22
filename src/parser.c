#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Forward declarations */
static Token *peek(Parser *parser);
static Token *advance(Parser *parser);
static int match(Parser *parser, TokenType type);
static Token *expect(Parser *parser, TokenType type);
static void skip_whitespace(Parser *parser);
static int is_at_end(Parser *parser);

static ASTNode *parse_program(Parser *parser);
static ASTNode *parse_function(Parser *parser);
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_block(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_expression_precedence(Parser *parser, int min_precedence);
static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_postfix(Parser *parser);
static ASTNode *parse_unary(Parser *parser);
static ASTNode *parse_if_statement(Parser *parser);
static ASTNode *parse_while_statement(Parser *parser);
static ASTNode *parse_for_statement(Parser *parser);
static ASTNode *parse_var_declaration(Parser *parser);
static ASTNode *parse_struct_definition(Parser *parser);
static ASTNode *parse_typedef(Parser *parser);
static ASTNode *parse_enum_definition(Parser *parser);
static int get_precedence(TokenType type);
static int is_binary_operator(TokenType type);
static int is_unary_operator(TokenType type);
static int is_type_keyword(TokenType type);

/*
 * parser_create - Create a new parser
 * @tokens: Array of tokens
 * @token_count: Number of tokens
 *
 * Return: Pointer to new parser, or NULL on failure
 */
Parser *parser_create(Token **tokens, int token_count)
{
	Parser *parser;

	if (!tokens || token_count <= 0)
		return (NULL);

	parser = malloc(sizeof(Parser));
	if (!parser)
		return (NULL);

	parser->tokens = tokens;
	parser->token_count = token_count;
	parser->current = 0;
	parser->error_count = 0;

	return (parser);
}

/*
 * parser_destroy - Free parser memory
 * @parser: Parser to destroy
 */
void parser_destroy(Parser *parser)
{
	if (!parser)
		return;

	free(parser);
}

/*
 * Helper functions for parser
 */

/*
 * is_at_end - Check if at end of token stream
 * @parser: Parser instance
 *
 * Return: 1 if at end, 0 otherwise
 */
static int is_at_end(Parser *parser)
{
	return (parser->current >= parser->token_count ||
		parser->tokens[parser->current]->type == TOK_EOF);
}

/*
 * peek - Get current token without consuming
 * @parser: Parser instance
 *
 * Return: Current token, or NULL if at end
 */
static Token *peek(Parser *parser)
{
	if (is_at_end(parser))
		return (NULL);
	return (parser->tokens[parser->current]);
}

/*
 * advance - Consume and return current token
 * @parser: Parser instance
 *
 * Return: Current token before advancing
 */
static Token *advance(Parser *parser)
{
	if (is_at_end(parser))
		return (NULL);
	return (parser->tokens[parser->current++]);
}

/*
 * match - Check if current token matches type
 * @parser: Parser instance
 * @type: Token type to match
 *
 * Return: 1 if matches, 0 otherwise
 */
static int match(Parser *parser, TokenType type)
{
	Token *token = peek(parser);

	if (!token)
		return (0);
	return (token->type == type);
}

/*
 * expect - Consume token and verify it matches expected type
 * @parser: Parser instance
 * @type: Expected token type
 *
 * Return: Token if matches, NULL on error
 */
static Token *expect(Parser *parser, TokenType type)
{
	Token *token = peek(parser);

	if (!token || token->type != type)
	{
		fprintf(stderr, "Parse error: expected %s, got %s\n",
			token_type_to_string(type),
			token ? token_type_to_string(token->type) : "EOF");
		parser->error_count++;
		return (NULL);
	}

	return (advance(parser));
}

/*
 * skip_whitespace - Skip whitespace and newline tokens
 * @parser: Parser instance
 */
static void skip_whitespace(Parser *parser)
{
	Token *token;

	while (!is_at_end(parser))
	{
		token = peek(parser);
		if (token->type == TOK_WHITESPACE ||
		    token->type == TOK_NEWLINE)
			advance(parser);
		else
			break;
	}
}

/*
 * Helper functions for expression parsing
 */

/*
 * is_type_keyword - Check if token is a type keyword
 */
static int is_type_keyword(TokenType type)
{
	return (type == TOK_INT || type == TOK_VOID || type == TOK_CHAR_KW ||
		type == TOK_LONG || type == TOK_SHORT || type == TOK_FLOAT_KW ||
		type == TOK_DOUBLE || type == TOK_UNSIGNED || type == TOK_SIGNED ||
		type == TOK_CONST || type == TOK_STATIC || type == TOK_STRUCT ||
		type == TOK_TYPEDEF);
}

/*
 * get_precedence - Get operator precedence (higher = tighter binding)
 */
static int get_precedence(TokenType type)
{
	switch (type)
	{
	case TOK_ASSIGN:
	case TOK_PLUS_ASSIGN:
	case TOK_MINUS_ASSIGN:
	case TOK_STAR_ASSIGN:
	case TOK_SLASH_ASSIGN:
	case TOK_PERCENT_ASSIGN:
		return (1); /* Assignment (right-to-left) */
	case TOK_LOGICAL_OR:
		return (2);
	case TOK_LOGICAL_AND:
		return (3);
	case TOK_PIPE:
		return (4);
	case TOK_CARET:
		return (5);
	case TOK_AMPERSAND:
		return (6);
	case TOK_EQUAL:
	case TOK_NOT_EQUAL:
		return (7);
	case TOK_LESS:
	case TOK_GREATER:
	case TOK_LESS_EQUAL:
	case TOK_GREATER_EQUAL:
		return (8);
	case TOK_LSHIFT:
	case TOK_RSHIFT:
		return (9);
	case TOK_PLUS:
	case TOK_MINUS:
		return (10);
	case TOK_STAR:
	case TOK_SLASH:
	case TOK_PERCENT:
		return (11);
	default:
		return (0);
	}
}

/*
 * is_binary_operator - Check if token is a binary operator
 */
static int is_binary_operator(TokenType type)
{
	return (get_precedence(type) > 0);
}

/*
 * is_unary_operator - Check if token is a unary operator
 */
static int is_unary_operator(TokenType type)
{
	return (type == TOK_LOGICAL_NOT || type == TOK_TILDE ||
		type == TOK_PLUS || type == TOK_MINUS || type == TOK_STAR ||
		type == TOK_AMPERSAND || type == TOK_INCREMENT ||
		type == TOK_DECREMENT);
}

/*
 * parse_primary - Parse primary expression (literals, identifiers, calls)
 */
static ASTNode *parse_primary(Parser *parser)
{
	Token *token;
	ASTNode *node;

	skip_whitespace(parser);
	token = peek(parser);

	if (!token)
		return (NULL);

	/* Literals */
	if (token->type == TOK_INTEGER || token->type == TOK_FLOAT ||
	    token->type == TOK_STRING || token->type == TOK_CHAR)
	{
		node = ast_node_create(NODE_LITERAL, token);
		advance(parser);
		return (node);
	}

	/* Identifiers and function calls */
	if (token->type == TOK_IDENTIFIER)
	{
		node = ast_node_create(NODE_IDENTIFIER, token);
		advance(parser);
		skip_whitespace(parser);

		/* Check for function call */
		if (match(parser, TOK_LPAREN))
		{
			ASTNode *call = ast_node_create(NODE_CALL, token);

			advance(parser); /* consume ( */
			skip_whitespace(parser);

			/* Parse arguments */
			while (!is_at_end(parser) && !match(parser, TOK_RPAREN))
			{
				ASTNode *arg = parse_expression(parser);

				if (arg)
					ast_node_add_child(call, arg);
				skip_whitespace(parser);
				if (match(parser, TOK_COMMA))
					advance(parser);
				skip_whitespace(parser);
			}

			expect(parser, TOK_RPAREN);
			ast_node_destroy(node);
			return (call);
		}

		/* Check for array access */
		if (match(parser, TOK_LBRACKET))
		{
			ASTNode *arr_access = ast_node_create(NODE_ARRAY_ACCESS, NULL);

			ast_node_add_child(arr_access, node);
			advance(parser); /* consume [ */
			ast_node_add_child(arr_access, parse_expression(parser));
			skip_whitespace(parser);
			expect(parser, TOK_RBRACKET);
			return (arr_access);
		}

		return (node);
	}

	/* Parenthesized expression */
	if (token->type == TOK_LPAREN)
	{
		advance(parser);
		node = parse_expression(parser);
		skip_whitespace(parser);
		expect(parser, TOK_RPAREN);
		return (node);
	}

	return (NULL);
}

/*
 * parse_postfix - Parse postfix expression (handle ++ and --)
 */
static ASTNode *parse_postfix(Parser *parser)
{
	ASTNode *node;
	Token *op_token;

	node = parse_primary(parser);
	if (!node)
		return (NULL);

	skip_whitespace(parser);
	op_token = peek(parser);

	/* Check for postfix ++ or -- */
	if (op_token && (op_token->type == TOK_INCREMENT ||
	    op_token->type == TOK_DECREMENT))
	{
		ASTNode *postfix = ast_node_create(NODE_UNARY, op_token);

		advance(parser);
		ast_node_add_child(postfix, node);
		return (postfix);
	}

	return (node);
}

/*
 * parse_unary - Parse unary expression
 */
static ASTNode *parse_unary(Parser *parser)
{
	Token *token;
	ASTNode *node, *operand;

	skip_whitespace(parser);
	token = peek(parser);

	if (!token)
		return (NULL);

	/* Check for unary operators */
	if (is_unary_operator(token->type))
	{
		node = ast_node_create(NODE_UNARY, token);
		advance(parser);
		operand = parse_unary(parser); /* Right-associative */
		if (operand)
			ast_node_add_child(node, operand);
		return (node);
	}

	/* Otherwise parse postfix */
	return (parse_postfix(parser));
}

/*
 * parse_expression_precedence - Parse expression with operator precedence
 */
static ASTNode *parse_expression_precedence(Parser *parser, int min_precedence)
{
	ASTNode *left, *right, *binary;
	Token *op_token;
	int precedence;

	/* Parse left side (unary or primary) */
	left = parse_unary(parser);
	if (!left)
		return (NULL);

	skip_whitespace(parser);

	/* Parse binary operators */
	while (!is_at_end(parser))
	{
		op_token = peek(parser);
		if (!op_token || !is_binary_operator(op_token->type))
			break;

		precedence = get_precedence(op_token->type);
		if (precedence < min_precedence)
			break;

		advance(parser); /* consume operator */
		skip_whitespace(parser);

		/* Right-associative for assignment operators */
		if (op_token->type >= TOK_ASSIGN &&
		    op_token->type <= TOK_RSHIFT_ASSIGN)
			right = parse_expression_precedence(parser, precedence);
		else
			right = parse_expression_precedence(parser, precedence + 1);

		if (!right)
		{
			ast_node_destroy(left);
			return (NULL);
		}

		/* Create binary node */
		binary = ast_node_create(NODE_BINARY, op_token);
		ast_node_add_child(binary, left);
		ast_node_add_child(binary, right);
		left = binary;

		skip_whitespace(parser);
	}

	return (left);
}

/*
 * parse_expression - Parse an expression
 */
static ASTNode *parse_expression(Parser *parser)
{
	return (parse_expression_precedence(parser, 0));
}

/*
 * parse_var_declaration - Parse variable declaration
 */
static ASTNode *parse_var_declaration(Parser *parser)
{
	Token *type_token, *name_token;
	ASTNode *node;

	skip_whitespace(parser);
	type_token = peek(parser);

	if (!type_token || !is_type_keyword(type_token->type))
		return (NULL);

	advance(parser); /* consume type */
	skip_whitespace(parser);

	/* Handle pointer declarations: int *ptr */
	while (match(parser, TOK_STAR))
	{
		advance(parser);
		skip_whitespace(parser);
	}

	name_token = expect(parser, TOK_IDENTIFIER);
	if (!name_token)
		return (NULL);

	node = ast_node_create(NODE_VAR_DECL, type_token);
	skip_whitespace(parser);

	/* Handle array declarations: int arr[] or int arr[10] */
	if (match(parser, TOK_LBRACKET))
	{
		advance(parser); /* consume [ */
		skip_whitespace(parser);

		/* Consume array size if present */
		if (!match(parser, TOK_RBRACKET))
		{
			parse_expression(parser); /* consume size expression */
			skip_whitespace(parser);
		}

		expect(parser, TOK_RBRACKET);
		skip_whitespace(parser);
	}

	/* Check for initialization */
	if (match(parser, TOK_ASSIGN))
	{
		advance(parser);
		ASTNode *init = parse_expression(parser);

		if (init)
			ast_node_add_child(node, init);
	}

	skip_whitespace(parser);

	/* Handle comma-separated declarations: int i, j, k; */
	while (match(parser, TOK_COMMA))
	{
		advance(parser); /* consume comma */
		skip_whitespace(parser);

		/* Handle pointer in comma list: int *p, *q */
		while (match(parser, TOK_STAR))
		{
			advance(parser);
			skip_whitespace(parser);
		}

		name_token = expect(parser, TOK_IDENTIFIER);
		if (!name_token)
			break;

		skip_whitespace(parser);

		/* Handle array in comma list */
		if (match(parser, TOK_LBRACKET))
		{
			advance(parser);
			skip_whitespace(parser);
			if (!match(parser, TOK_RBRACKET))
			{
				parse_expression(parser);
				skip_whitespace(parser);
			}
			expect(parser, TOK_RBRACKET);
			skip_whitespace(parser);
		}

		/* Check for initialization of this variable */
		if (match(parser, TOK_ASSIGN))
		{
			advance(parser);
			parse_expression(parser); /* consume but don't store for now */
			skip_whitespace(parser);
		}
	}

	expect(parser, TOK_SEMICOLON);

	return (node);
}

/*
 * parse_if_statement - Parse if statement
 */
static ASTNode *parse_if_statement(Parser *parser)
{
	ASTNode *node, *condition, *then_branch, *else_branch;

	advance(parser); /* consume 'if' */
	skip_whitespace(parser);

	if (!expect(parser, TOK_LPAREN))
		return (NULL);

	condition = parse_expression(parser);
	skip_whitespace(parser);

	if (!expect(parser, TOK_RPAREN))
	{
		ast_node_destroy(condition);
		return (NULL);
	}

	skip_whitespace(parser);

	then_branch = parse_statement(parser);

	node = ast_node_create(NODE_IF, NULL);
	if (condition)
		ast_node_add_child(node, condition);
	if (then_branch)
		ast_node_add_child(node, then_branch);

	skip_whitespace(parser);

	/* Check for else */
	if (match(parser, TOK_ELSE))
	{
		advance(parser);
		skip_whitespace(parser);
		else_branch = parse_statement(parser);
		if (else_branch)
			ast_node_add_child(node, else_branch);
	}

	return (node);
}

/*
 * parse_while_statement - Parse while loop
 */
static ASTNode *parse_while_statement(Parser *parser)
{
	ASTNode *node, *condition, *body;

	advance(parser); /* consume 'while' */
	skip_whitespace(parser);

	if (!expect(parser, TOK_LPAREN))
		return (NULL);

	condition = parse_expression(parser);
	skip_whitespace(parser);

	if (!expect(parser, TOK_RPAREN))
	{
		ast_node_destroy(condition);
		return (NULL);
	}

	skip_whitespace(parser);
	body = parse_statement(parser);

	node = ast_node_create(NODE_WHILE, NULL);
	if (condition)
		ast_node_add_child(node, condition);
	if (body)
		ast_node_add_child(node, body);

	return (node);
}

/*
 * parse_for_statement - Parse for loop
 */
static ASTNode *parse_for_statement(Parser *parser)
{
	ASTNode *node, *init, *condition, *increment, *body;

	advance(parser); /* consume 'for' */
	skip_whitespace(parser);

	if (!expect(parser, TOK_LPAREN))
		return (NULL);

	node = ast_node_create(NODE_FOR, NULL);

	/* Initialization */
	skip_whitespace(parser);
	if (!match(parser, TOK_SEMICOLON))
	{
		init = parse_expression(parser);
		if (init)
			ast_node_add_child(node, init);
	}
	skip_whitespace(parser);
	expect(parser, TOK_SEMICOLON);

	/* Condition */
	skip_whitespace(parser);
	if (!match(parser, TOK_SEMICOLON))
	{
		condition = parse_expression(parser);
		if (condition)
			ast_node_add_child(node, condition);
	}
	skip_whitespace(parser);
	expect(parser, TOK_SEMICOLON);

	/* Increment */
	skip_whitespace(parser);
	if (!match(parser, TOK_RPAREN))
	{
		increment = parse_expression(parser);
		if (increment)
			ast_node_add_child(node, increment);
	}
	skip_whitespace(parser);
	expect(parser, TOK_RPAREN);

	/* Body */
	skip_whitespace(parser);
	body = parse_statement(parser);
	if (body)
		ast_node_add_child(node, body);

	return (node);
}

/*
 * parse_block - Parse a block statement { ... }
 * @parser: Parser instance
 *
 * Return: Block AST node, or NULL on error
 */
static ASTNode *parse_block(Parser *parser)
{
	ASTNode *block, *stmt;

	skip_whitespace(parser);

	if (!expect(parser, TOK_LBRACE))
		return (NULL);

	block = ast_node_create(NODE_BLOCK, NULL);
	if (!block)
		return (NULL);

	skip_whitespace(parser);

	/* Parse statements until we hit } */
	while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
	{
		skip_whitespace(parser);

		if (match(parser, TOK_RBRACE))
			break;

		stmt = parse_statement(parser);
		if (stmt)
			ast_node_add_child(block, stmt);

		skip_whitespace(parser);
	}

	skip_whitespace(parser);
	expect(parser, TOK_RBRACE);

	return (block);
}

/*
 * parse_statement - Parse a statement
 * @parser: Parser instance
 *
 * Return: Statement AST node, or NULL on error
 */
static ASTNode *parse_statement(Parser *parser)
{
	Token *token;
	ASTNode *node;

	skip_whitespace(parser);
	token = peek(parser);

	if (!token)
		return (NULL);

	/* Control flow statements */
	if (token->type == TOK_IF)
		return (parse_if_statement(parser));

	if (token->type == TOK_WHILE)
		return (parse_while_statement(parser));

	if (token->type == TOK_FOR)
		return (parse_for_statement(parser));

	/* Return statement */
	if (token->type == TOK_RETURN)
	{
		advance(parser);
		node = ast_node_create(NODE_RETURN, token);
		skip_whitespace(parser);

		/* Parse return value if present */
		if (!match(parser, TOK_SEMICOLON))
		{
			ASTNode *expr = parse_expression(parser);

			if (expr)
				ast_node_add_child(node, expr);
		}

		skip_whitespace(parser);
		expect(parser, TOK_SEMICOLON);
		return (node);
	}

	/* Break statement */
	if (token->type == TOK_BREAK)
	{
		node = ast_node_create(NODE_BREAK, token);
		advance(parser);
		skip_whitespace(parser);
		expect(parser, TOK_SEMICOLON);
		return (node);
	}

	/* Continue statement */
	if (token->type == TOK_CONTINUE)
	{
		node = ast_node_create(NODE_CONTINUE, token);
		advance(parser);
		skip_whitespace(parser);
		expect(parser, TOK_SEMICOLON);
		return (node);
	}

	/* Block statement */
	if (token->type == TOK_LBRACE)
		return (parse_block(parser));

	/* Variable declaration */
	if (is_type_keyword(token->type))
		return (parse_var_declaration(parser));

	/* Expression statement */
	node = ast_node_create(NODE_EXPR_STMT, NULL);
	if (node)
	{
		ASTNode *expr = parse_expression(parser);

		if (expr)
			ast_node_add_child(node, expr);
		skip_whitespace(parser);
		expect(parser, TOK_SEMICOLON);
	}

	return (node);
}

/*
 * parse_struct_definition - Parse struct definition
 */
static ASTNode *parse_struct_definition(Parser *parser)
{
	Token *name_token = NULL;
	ASTNode *node;

	advance(parser); /* consume 'struct' */
	skip_whitespace(parser);

	/* Struct name is optional (for anonymous structs) */
	if (match(parser, TOK_IDENTIFIER))
	{
		name_token = peek(parser);
		advance(parser);
		skip_whitespace(parser);
	}

	node = ast_node_create(NODE_STRUCT, name_token);

	/* Parse struct body if present */
	if (match(parser, TOK_LBRACE))
	{
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse struct members */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			skip_whitespace(parser);
			if (match(parser, TOK_RBRACE))
				break;

			/* Parse member declaration (simplified - just consume until semicolon) */
			while (!is_at_end(parser) && !match(parser, TOK_SEMICOLON))
			{
				advance(parser);
				skip_whitespace(parser);
			}

			if (match(parser, TOK_SEMICOLON))
				advance(parser);
			skip_whitespace(parser);
		}

		expect(parser, TOK_RBRACE);
	}

	return (node);
}

/*
 * parse_enum_definition - Parse enum definition
 */
static ASTNode *parse_enum_definition(Parser *parser)
{
	Token *name_token = NULL;
	ASTNode *node;

	advance(parser); /* consume 'enum' */
	skip_whitespace(parser);

	/* Enum name is optional */
	if (match(parser, TOK_IDENTIFIER))
	{
		name_token = peek(parser);
		advance(parser);
		skip_whitespace(parser);
	}

	node = ast_node_create(NODE_ENUM, name_token);

	/* Parse enum body if present */
	if (match(parser, TOK_LBRACE))
	{
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse enum values (simplified - just consume until closing brace) */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			advance(parser);
			skip_whitespace(parser);
		}

		expect(parser, TOK_RBRACE);
	}

	return (node);
}

/*
 * parse_typedef - Parse typedef declaration
 */
static ASTNode *parse_typedef(Parser *parser)
{
	ASTNode *node, *inner;
	Token *alias_token;

	advance(parser); /* consume 'typedef' */
	skip_whitespace(parser);

	node = ast_node_create(NODE_TYPEDEF, NULL);

	/* Check if it's typedef struct or typedef enum */
	if (match(parser, TOK_STRUCT))
	{
		inner = parse_struct_definition(parser);
		if (inner)
			ast_node_add_child(node, inner);
		skip_whitespace(parser);
		/* Get the typedef alias name after the struct */
		if (match(parser, TOK_IDENTIFIER))
		{
			node->token = peek(parser);
			advance(parser);
		}
	}
	else if (match(parser, TOK_ENUM))
	{
		inner = parse_enum_definition(parser);
		if (inner)
			ast_node_add_child(node, inner);
		skip_whitespace(parser);
		/* Get the typedef alias name after the enum */
		if (match(parser, TOK_IDENTIFIER))
		{
			node->token = peek(parser);
			advance(parser);
		}
	}
	else
	{
		/* Regular typedef - consume until we find the alias name */
		while (!is_at_end(parser) && !match(parser, TOK_SEMICOLON))
		{
			/* The last identifier before semicolon is the typedef name */
			if (match(parser, TOK_IDENTIFIER))
			{
				Token *next;

				alias_token = peek(parser);
				advance(parser);
				skip_whitespace(parser);
				next = peek(parser);
				/* If next is semicolon, this was the alias */
				if (next && next->type == TOK_SEMICOLON)
				{
					node->token = alias_token;
					break;
				}
			}
			else
			{
				advance(parser);
				skip_whitespace(parser);
			}
		}
	}

	skip_whitespace(parser);
	expect(parser, TOK_SEMICOLON);

	return (node);
}

/*
 * parse_function - Parse a function declaration
 * @parser: Parser instance
 *
 * Return: Function AST node, or NULL on error
 */
static ASTNode *parse_function(Parser *parser)
{
	Token *return_type, *name;
	ASTNode *func, *body;

	skip_whitespace(parser);

	/* Parse return type (simplified - just consume type keywords) */
	return_type = peek(parser);
	if (!return_type)
		return (NULL);

	/* Handle type modifiers: unsigned, signed, static, const, etc. */
	while (return_type && (return_type->type == TOK_UNSIGNED ||
	       return_type->type == TOK_SIGNED ||
	       return_type->type == TOK_STATIC ||
	       return_type->type == TOK_CONST))
	{
		advance(parser);
		skip_whitespace(parser);
		return_type = peek(parser);
	}

	/* Accept base type keywords OR identifiers (for typedef'd types like node_t) */
	if (return_type && (return_type->type == TOK_INT ||
	    return_type->type == TOK_VOID ||
	    return_type->type == TOK_CHAR_KW ||
	    return_type->type == TOK_LONG ||
	    return_type->type == TOK_SHORT ||
	    return_type->type == TOK_FLOAT_KW ||
	    return_type->type == TOK_DOUBLE ||
	    return_type->type == TOK_IDENTIFIER))
	{
		advance(parser);
	}
	else
	{
		return (NULL);
	}

	skip_whitespace(parser);

	/* Handle pointer types: int *func() or node_t *func() */
	while (match(parser, TOK_STAR))
	{
		advance(parser);
		skip_whitespace(parser);
	}

	/* Parse function name */
	name = expect(parser, TOK_IDENTIFIER);
	if (!name)
		return (NULL);

	func = ast_node_create(NODE_FUNCTION, name);
	if (!func)
		return (NULL);

	skip_whitespace(parser);

	/* Parse parameter list */
	if (!expect(parser, TOK_LPAREN))
	{
		ast_node_destroy(func);
		return (NULL);
	}

	skip_whitespace(parser);

	/* Skip parameters for now (simplified) */
	while (!is_at_end(parser) && !match(parser, TOK_RPAREN))
	{
		advance(parser);
		skip_whitespace(parser);
	}

	if (!expect(parser, TOK_RPAREN))
	{
		ast_node_destroy(func);
		return (NULL);
	}

	skip_whitespace(parser);

	/* Parse function body */
	body = parse_block(parser);
	if (body)
		ast_node_add_child(func, body);

	return (func);
}

/*
 * parse_program - Parse entire program
 * @parser: Parser instance
 *
 * Return: Program AST node, or NULL on error
 */
static ASTNode *parse_program(Parser *parser)
{
	ASTNode *program, *func;

	program = ast_node_create(NODE_PROGRAM, NULL);
	if (!program)
		return (NULL);

	while (!is_at_end(parser))
	{
		skip_whitespace(parser);

		if (is_at_end(parser))
			break;

		/* Skip preprocessor directives and comments for now */
		if (match(parser, TOK_PREPROCESSOR) ||
		    match(parser, TOK_COMMENT_LINE) ||
		    match(parser, TOK_COMMENT_BLOCK))
		{
			advance(parser);
			continue;
		}

		/* Try to parse typedef */
		if (match(parser, TOK_TYPEDEF))
		{
			func = parse_typedef(parser);
			if (func)
				ast_node_add_child(program, func);
			continue;
		}

		/* Try to parse struct */
		if (match(parser, TOK_STRUCT))
		{
			func = parse_struct_definition(parser);
			if (func)
			{
				ast_node_add_child(program, func);
				skip_whitespace(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
			continue;
		}

		/* Try to parse enum */
		if (match(parser, TOK_ENUM))
		{
			func = parse_enum_definition(parser);
			if (func)
			{
				ast_node_add_child(program, func);
				skip_whitespace(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
			continue;
		}

		/* Try to parse a function */
		func = parse_function(parser);
		if (func)
		{
			ast_node_add_child(program, func);
		}
		else
		{
			/* If parsing failed and no errors, skip to next top-level declaration */
			/* This handles typedef, struct, enum, etc. that we don't parse yet */
			if (parser->error_count > 0)
				break;

			/* Skip to next semicolon or closing brace at top level */
			while (!is_at_end(parser))
			{
				Token *t = peek(parser);

				if (t->type == TOK_SEMICOLON)
				{
					advance(parser);
					break;
				}
				if (t->type == TOK_LBRACE)
				{
					/* Skip entire block */
					int brace_count = 0;

					while (!is_at_end(parser))
					{
						t = peek(parser);
						if (t->type == TOK_LBRACE)
							brace_count++;
						else if (t->type == TOK_RBRACE)
						{
							brace_count--;
							advance(parser);
							if (brace_count == 0)
								break;
							continue;
						}
						advance(parser);
					}
					/* Skip trailing semicolon if present */
					skip_whitespace(parser);
					if (match(parser, TOK_SEMICOLON))
						advance(parser);
					break;
				}
				advance(parser);
			}
		}

		skip_whitespace(parser);
	}

	return (program);
}

/*
 * parser_parse - Parse tokens into AST
 * @parser: Parser instance
 *
 * Return: Root AST node, or NULL on error
 */
ASTNode *parser_parse(Parser *parser)
{
	if (!parser)
		return (NULL);

	return (parse_program(parser));
}
