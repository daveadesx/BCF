#include "../include/parser.h"
#include "../include/symbol_table.h"
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
static ASTNode *parse_switch_statement(Parser *parser);
static ASTNode *parse_do_while_statement(Parser *parser);
static ASTNode *parse_var_declaration(Parser *parser);
static ASTNode *parse_struct_definition(Parser *parser);
static ASTNode *parse_typedef(Parser *parser);
static ASTNode *parse_enum_definition(Parser *parser);
static ASTNode *parse_union_definition(Parser *parser);
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
	parser->symbols = symbol_table_create(NULL);

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

	if (parser->symbols)
		symbol_table_destroy(parser->symbols);

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
 * peek_ahead - Look ahead n tokens (skipping whitespace/newlines/comments)
 * @parser: Parser instance
 * @n: How many non-whitespace tokens to look ahead (0 = current)
 *
 * Return: Token at position, or NULL if at end
 */
static Token *peek_ahead(Parser *parser, int n)
{
	int pos = parser->current;
	int count = 0;

	while (pos < parser->token_count)
	{
		Token *t = parser->tokens[pos];

		if (t->type != TOK_WHITESPACE && t->type != TOK_NEWLINE &&
		    t->type != TOK_COMMENT_LINE && t->type != TOK_COMMENT_BLOCK)
		{
			if (count == n)
				return (t);
			count++;
		}
		pos++;
	}
	return (NULL);
}

/*
 * looks_like_ptr_declaration - Check if tokens look like "Type *var"
 * @parser: Parser instance
 *
 * Heuristic: IDENTIFIER STAR IDENTIFIER followed by ; or , or = or [
 * Return: 1 if looks like declaration, 0 otherwise
 */
static int looks_like_ptr_declaration(Parser *parser)
{
	Token *t0 = peek_ahead(parser, 0);
	Token *t1 = peek_ahead(parser, 1);
	Token *t2 = peek_ahead(parser, 2);
	Token *t3 = peek_ahead(parser, 3);

	if (!t0 || !t1 || !t2)
		return (0);

	/* Pattern: IDENTIFIER * IDENTIFIER (;|,|=|[) */
	if (t0->type == TOK_IDENTIFIER &&
	    t1->type == TOK_STAR &&
	    t2->type == TOK_IDENTIFIER &&
	    t3 && (t3->type == TOK_SEMICOLON || t3->type == TOK_COMMA ||
		   t3->type == TOK_ASSIGN || t3->type == TOK_LBRACKET))
	{
		return (1);
	}

	return (0);
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
 * sync_to_semicolon - Skip tokens until we find a semicolon or closing brace
 * @parser: Parser instance
 *
 * Used for error recovery to skip past malformed statements
 */
static void sync_to_semicolon(Parser *parser)
{
	while (!is_at_end(parser))
	{
		Token *token = peek(parser);

		if (!token)
			break;
		if (token->type == TOK_SEMICOLON)
		{
			advance(parser);
			break;
		}
		if (token->type == TOK_RBRACE)
			break;
		advance(parser);
	}
}

/*
 * skip_whitespace - Skip whitespace, newlines, and comments
 * @parser: Parser instance
 */
static void skip_whitespace(Parser *parser)
{
	Token *token;

	while (!is_at_end(parser))
	{
		token = peek(parser);
		if (token->type == TOK_WHITESPACE ||
		    token->type == TOK_NEWLINE ||
		    token->type == TOK_COMMENT_LINE ||
		    token->type == TOK_COMMENT_BLOCK)
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
		type == TOK_TYPEDEF || type == TOK_EXTERN);
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
	case TOK_AMPERSAND_ASSIGN:
	case TOK_PIPE_ASSIGN:
	case TOK_CARET_ASSIGN:
	case TOK_LSHIFT_ASSIGN:
	case TOK_RSHIFT_ASSIGN:
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

	/* Parenthesized expression or type cast */
	if (token->type == TOK_LPAREN)
	{
		advance(parser);
		skip_whitespace(parser);
		token = peek(parser);

		/* Check if this is a type cast */
		if (token && is_type_keyword(token->type))
		{
			/* Type cast: (type)expression */
			ASTNode *cast_node = ast_node_create(NODE_CAST, token);

			/* Skip over the type (including any pointer stars) */
			while (!is_at_end(parser))
			{
				token = peek(parser);
				if (!token)
					break;
				if (token->type == TOK_RPAREN)
					break;
				advance(parser);
				skip_whitespace(parser);
			}

			expect(parser, TOK_RPAREN);
			skip_whitespace(parser);

			/* Parse the expression being cast */
			node = parse_unary(parser);
			if (node)
				ast_node_add_child(cast_node, node);
			return (cast_node);
		}

		/* Regular parenthesized expression */
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
	ASTNode *node = NULL;
	Token *token = NULL;

	node = parse_primary(parser);
	if (!node)
		return (NULL);

	/* Handle repeated postfix operations: array access, calls, member access, postfix ++/-- */
	for (;;)
	{
		skip_whitespace(parser);
		token = peek(parser);
		if (!token)
			break;

		/* Array access: expr[ index ] */
		if (token->type == TOK_LBRACKET)
		{
			ASTNode *arr_access = ast_node_create(NODE_ARRAY_ACCESS, NULL);

			advance(parser); /* consume [ */
			ast_node_add_child(arr_access, node);
			skip_whitespace(parser);
			ast_node_add_child(arr_access, parse_expression(parser));
			skip_whitespace(parser);
			expect(parser, TOK_RBRACKET);
			node = arr_access;
			continue;
		}

		/* Function call: expr(args...) */
		if (token->type == TOK_LPAREN)
		{
			ASTNode *call = ast_node_create(NODE_CALL, NULL);

			advance(parser); /* consume ( */
			/* callee as first child */
			ast_node_add_child(call, node);
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
			node = call;
			continue;
		}

		/* Member access: .identifier or ->identifier */
		if (token->type == TOK_DOT || token->type == TOK_ARROW)
		{
			ASTNode *member = NULL;
			Token *name_token = NULL;

			advance(parser); /* consume . or -> */
			skip_whitespace(parser);

			/* Expect an identifier for the member name */
			name_token = expect(parser, TOK_IDENTIFIER);
			if (!name_token)
				return (NULL);

			member = ast_node_create(NODE_MEMBER_ACCESS, name_token);
			/* first child is the object, second implicitly the name via token */
			ast_node_add_child(member, node);
			node = member;
			continue;
		}

		/* Postfix ++ or -- */
		if (token->type == TOK_INCREMENT || token->type == TOK_DECREMENT)
		{
			ASTNode *postfix = ast_node_create(NODE_UNARY, token);
			advance(parser);
			ast_node_add_child(postfix, node);
			node = postfix;
			continue;
		}

		break;
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

	/* Handle sizeof as a special unary operator */
	if (token->type == TOK_SIZEOF)
	{
		node = ast_node_create(NODE_SIZEOF, token);
		advance(parser);
		skip_whitespace(parser);

		/* sizeof can be followed by a parenthesized type or expression */
		if (match(parser, TOK_LPAREN))
		{
			Token *next;

			advance(parser);
			skip_whitespace(parser);
			next = peek(parser);

			/* Check if it's sizeof(type) - type is a keyword or typedef */
			if (next && (is_type_keyword(next->type) ||
			    (next->type == TOK_IDENTIFIER &&
			     symbol_is_typedef(parser->symbols, next->lexeme))))
			{
				/* Store the type token in data field */
				node->data = next;

				/* Consume the type (possibly multiple tokens like unsigned int) */
				while (!is_at_end(parser) && !match(parser, TOK_RPAREN))
				{
					advance(parser);
					skip_whitespace(parser);
				}
				expect(parser, TOK_RPAREN);
			}
			else
			{
				/* It's sizeof(expression) */
				operand = parse_expression(parser);
				if (operand)
					ast_node_add_child(node, operand);
				skip_whitespace(parser);
				expect(parser, TOK_RPAREN);
			}
		}
		else
		{
			operand = parse_unary(parser);
			if (operand)
				ast_node_add_child(node, operand);
		}

		return (node);
	}

	/* Check for other unary operators */
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
		if (!op_token)
			break;

		/* Handle ternary operator */
		if (op_token->type == TOK_QUESTION)
		{
			ASTNode *ternary, *then_expr, *else_expr;

			advance(parser); /* consume ? */
			skip_whitespace(parser);

			then_expr = parse_expression(parser);
			skip_whitespace(parser);

			if (!expect(parser, TOK_COLON))
			{
				ast_node_destroy(left);
				ast_node_destroy(then_expr);
				return (NULL);
			}

			skip_whitespace(parser);
			else_expr = parse_expression_precedence(parser, min_precedence);

			ternary = ast_node_create(NODE_TERNARY, op_token);
			ast_node_add_child(ternary, left);
			if (then_expr)
				ast_node_add_child(ternary, then_expr);
			if (else_expr)
				ast_node_add_child(ternary, else_expr);

			left = ternary;
			skip_whitespace(parser);
			continue;
		}

		if (!is_binary_operator(op_token->type))
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
	VarDeclData *var_data;
	Token **type_tokens = NULL;
	int type_count = 0;
	int type_capacity = 8;
	Token **array_tokens = NULL;
	int array_count = 0;
	int array_capacity = 4;

	skip_whitespace(parser);
	type_token = peek(parser);

	if (!type_token)
		return (NULL);

	type_tokens = malloc(sizeof(Token *) * type_capacity);
	if (!type_tokens)
		return (NULL);

	/* Check if it's a built-in type keyword */
	if (is_type_keyword(type_token->type))
	{
		type_tokens[type_count++] = advance(parser);
		skip_whitespace(parser);

		/* Handle struct/enum name (e.g., "struct node" or "enum color") */
		if ((type_token->type == TOK_STRUCT || type_token->type == TOK_ENUM) &&
		    match(parser, TOK_IDENTIFIER))
		{
			if (type_count >= type_capacity)
			{
				type_capacity *= 2;
				type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
			}
			type_tokens[type_count++] = advance(parser);
			skip_whitespace(parser);
		}

		/* Handle compound types: unsigned int, long long, const static int, etc. */
		while (peek(parser) && is_type_keyword(peek(parser)->type))
		{
			if (type_count >= type_capacity)
			{
				type_capacity *= 2;
				type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
			}
			type_tokens[type_count++] = advance(parser);
			skip_whitespace(parser);

			/* Handle struct/enum name after type keyword */
			if ((type_tokens[type_count - 1]->type == TOK_STRUCT ||
			     type_tokens[type_count - 1]->type == TOK_ENUM) &&
			    match(parser, TOK_IDENTIFIER))
			{
				if (type_count >= type_capacity)
				{
					type_capacity *= 2;
					type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
				}
				type_tokens[type_count++] = advance(parser);
				skip_whitespace(parser);
			}
		}

		/* After modifiers like static/const, check for typedef'd type */
		if (peek(parser) && peek(parser)->type == TOK_IDENTIFIER &&
		    symbol_is_typedef(parser->symbols, peek(parser)->lexeme))
		{
			if (type_count >= type_capacity)
			{
				type_capacity *= 2;
				type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
			}
			type_tokens[type_count++] = advance(parser);
			skip_whitespace(parser);
		}
	}
	/* Check if it's a typedef'd type */
	else if (type_token->type == TOK_IDENTIFIER &&
		 symbol_is_typedef(parser->symbols, type_token->lexeme))
	{
		type_tokens[type_count++] = advance(parser);
		skip_whitespace(parser);
	}
	/* Heuristic: unknown identifier that looks like a type (from headers) */
	else if (type_token->type == TOK_IDENTIFIER &&
		 looks_like_ptr_declaration(parser))
	{
		type_tokens[type_count++] = advance(parser);
		skip_whitespace(parser);
	}
	else
	{
		free(type_tokens);
		return (NULL);
	}

	/* Handle pointer declarations: int *ptr or node_t *node */
	while (match(parser, TOK_STAR))
	{
		if (type_count >= type_capacity)
		{
			type_capacity *= 2;
			type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
		}
		type_tokens[type_count++] = advance(parser);
		skip_whitespace(parser);
	}

	name_token = expect(parser, TOK_IDENTIFIER);
	if (!name_token)
	{
		free(type_tokens);
		return (NULL);
	}

	node = ast_node_create(NODE_VAR_DECL, type_token);
	skip_whitespace(parser);

	/* Handle array declarations: int arr[] or int arr[10] */
	array_tokens = malloc(sizeof(Token *) * array_capacity);
	while (match(parser, TOK_LBRACKET))
	{
		if (array_count >= array_capacity)
		{
			array_capacity *= 2;
			array_tokens = realloc(array_tokens, sizeof(Token *) * array_capacity);
		}
		array_tokens[array_count++] = advance(parser); /* [ */
		skip_whitespace(parser);

		/* Consume array size if present */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACKET))
		{
			if (array_count >= array_capacity)
			{
				array_capacity *= 2;
				array_tokens = realloc(array_tokens, sizeof(Token *) * array_capacity);
			}
			array_tokens[array_count++] = advance(parser);
			skip_whitespace(parser);
		}

		if (match(parser, TOK_RBRACKET))
		{
			if (array_count >= array_capacity)
			{
				array_capacity *= 2;
				array_tokens = realloc(array_tokens, sizeof(Token *) * array_capacity);
			}
			array_tokens[array_count++] = advance(parser); /* ] */
		}
		skip_whitespace(parser);
	}

	/* Create VarDeclData */
	var_data = malloc(sizeof(VarDeclData));
	if (var_data)
	{
		var_data->type_tokens = type_tokens;
		var_data->type_count = type_count;
		var_data->name_token = name_token;
		var_data->array_tokens = array_tokens;
		var_data->array_count = array_count;
		var_data->extra_vars = NULL;
		var_data->extra_count = 0;
		var_data->init_expr = NULL;
		node->data = var_data;
	}
	else
	{
		free(type_tokens);
		free(array_tokens);
	}

	/* Check for initialization */
	if (match(parser, TOK_ASSIGN))
	{
		advance(parser);
		ASTNode *init = parse_expression(parser);

		if (init)
		{
			ast_node_add_child(node, init);
			if (var_data)
				var_data->init_expr = init;
		}
	}

	skip_whitespace(parser);

	/* Handle comma-separated declarations: int i, j, k; */
	if (match(parser, TOK_COMMA) && var_data)
	{
		int extra_capacity = 4;
		VarDeclData **extras = malloc(sizeof(VarDeclData *) * extra_capacity);
		int extra_count = 0;

		while (match(parser, TOK_COMMA))
		{
			VarDeclData *extra;
			Token **extra_type_tokens;
			int extra_type_count = 0;
			Token **extra_arr_tokens = NULL;
			int extra_arr_count = 0;
			int i;

			advance(parser); /* consume comma */
			skip_whitespace(parser);

			/* Copy base type tokens, then add any pointers */
			extra_type_tokens = malloc(sizeof(Token *) * (type_count + 4));
			for (i = 0; i < type_count; i++)
			{
				/* Copy type but stop at pointers */
				if (type_tokens[i]->type == TOK_STAR)
					break;
				extra_type_tokens[extra_type_count++] = type_tokens[i];
			}

			/* Handle pointer in comma list: int *p, *q */
			while (match(parser, TOK_STAR))
			{
				extra_type_tokens[extra_type_count++] = advance(parser);
				skip_whitespace(parser);
			}

			name_token = expect(parser, TOK_IDENTIFIER);
			if (!name_token)
			{
				free(extra_type_tokens);
				break;
			}

			skip_whitespace(parser);

			/* Handle array in comma list */
			if (match(parser, TOK_LBRACKET))
			{
				int arr_cap = 4;

				extra_arr_tokens = malloc(sizeof(Token *) * arr_cap);
				while (match(parser, TOK_LBRACKET))
				{
					extra_arr_tokens[extra_arr_count++] = advance(parser);
					skip_whitespace(parser);

					while (!is_at_end(parser) && !match(parser, TOK_RBRACKET))
					{
						if (extra_arr_count >= arr_cap)
						{
							arr_cap *= 2;
							extra_arr_tokens = realloc(extra_arr_tokens,
								sizeof(Token *) * arr_cap);
						}
						extra_arr_tokens[extra_arr_count++] = advance(parser);
						skip_whitespace(parser);
					}

					if (match(parser, TOK_RBRACKET))
						extra_arr_tokens[extra_arr_count++] = advance(parser);
					skip_whitespace(parser);
				}
			}

			/* Create extra VarDeclData */
			extra = malloc(sizeof(VarDeclData));
			if (extra)
			{
				extra->type_tokens = extra_type_tokens;
				extra->type_count = extra_type_count;
				extra->name_token = name_token;
				extra->array_tokens = extra_arr_tokens;
				extra->array_count = extra_arr_count;
				extra->extra_vars = NULL;
				extra->extra_count = 0;
				extra->init_expr = NULL;

				/* Check for initialization of this variable */
				if (match(parser, TOK_ASSIGN))
				{
					advance(parser);
					extra->init_expr = parse_expression(parser);
					skip_whitespace(parser);
				}

				if (extra_count >= extra_capacity)
				{
					extra_capacity *= 2;
					extras = realloc(extras, sizeof(VarDeclData *) * extra_capacity);
				}
				extras[extra_count++] = extra;
			}
			else
			{
				free(extra_type_tokens);
				free(extra_arr_tokens);
			}

			skip_whitespace(parser);
		}

		var_data->extra_vars = extras;
		var_data->extra_count = extra_count;
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

	/* Initialization - handle comma expressions like: i = 0, j = n - 1 */
	skip_whitespace(parser);
	if (!match(parser, TOK_SEMICOLON))
	{
		init = parse_expression(parser);
		if (init)
			ast_node_add_child(node, init);
		skip_whitespace(parser);
		/* Handle comma operator in for init */
		while (match(parser, TOK_COMMA))
		{
			advance(parser);
			skip_whitespace(parser);
			init = parse_expression(parser);
			if (init)
				ast_node_add_child(node, init);
			skip_whitespace(parser);
		}
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

	/* Increment - handle comma expressions like: i++, j-- */
	skip_whitespace(parser);
	if (!match(parser, TOK_RPAREN))
	{
		increment = parse_expression(parser);
		if (increment)
			ast_node_add_child(node, increment);
		skip_whitespace(parser);
		/* Handle comma operator in for increment */
		while (match(parser, TOK_COMMA))
		{
			advance(parser);
			skip_whitespace(parser);
			increment = parse_expression(parser);
			if (increment)
				ast_node_add_child(node, increment);
			skip_whitespace(parser);
		}
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
 * parse_switch_statement - Parse switch statement
 * @parser: Parser instance
 *
 * Return: Switch AST node, or NULL on error
 */
static ASTNode *parse_switch_statement(Parser *parser)
{
	ASTNode *node, *expr, *case_node;
	Token *token;

	advance(parser); /* consume 'switch' */
	skip_whitespace(parser);

	if (!expect(parser, TOK_LPAREN))
		return (NULL);

	expr = parse_expression(parser);
	skip_whitespace(parser);

	if (!expect(parser, TOK_RPAREN))
	{
		ast_node_destroy(expr);
		return (NULL);
	}

	node = ast_node_create(NODE_SWITCH, NULL);
	if (expr)
		ast_node_add_child(node, expr);

	skip_whitespace(parser);

	/* Parse switch body */
	if (!expect(parser, TOK_LBRACE))
	{
		ast_node_destroy(node);
		return (NULL);
	}

	skip_whitespace(parser);

	/* Parse cases and default */
	while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
	{
		skip_whitespace(parser);
		token = peek(parser);

		if (!token)
			break;

		if (token->type == TOK_CASE)
		{
			advance(parser); /* consume 'case' */
			skip_whitespace(parser);

			case_node = ast_node_create(NODE_CASE, token);

			/* Parse case value */
			ASTNode *case_val = parse_expression(parser);
			if (case_val)
				ast_node_add_child(case_node, case_val);

			skip_whitespace(parser);
			expect(parser, TOK_COLON);
			skip_whitespace(parser);

			/* Parse statements until next case/default/rbrace */
			while (!is_at_end(parser) &&
			       !match(parser, TOK_CASE) &&
			       !match(parser, TOK_DEFAULT) &&
			       !match(parser, TOK_RBRACE))
			{
				ASTNode *stmt = parse_statement(parser);
				if (stmt)
					ast_node_add_child(case_node, stmt);
				skip_whitespace(parser);
			}

			ast_node_add_child(node, case_node);
		}
		else if (token->type == TOK_DEFAULT)
		{
			advance(parser); /* consume 'default' */
			skip_whitespace(parser);
			expect(parser, TOK_COLON);
			skip_whitespace(parser);

			case_node = ast_node_create(NODE_CASE, token);

			/* Parse statements until next case/rbrace */
			while (!is_at_end(parser) &&
			       !match(parser, TOK_CASE) &&
			       !match(parser, TOK_RBRACE))
			{
				ASTNode *stmt = parse_statement(parser);
				if (stmt)
					ast_node_add_child(case_node, stmt);
				skip_whitespace(parser);
			}

			ast_node_add_child(node, case_node);
		}
		else
		{
			/* Skip unexpected tokens */
			advance(parser);
		}
	}

	expect(parser, TOK_RBRACE);

	return (node);
}

/*
 * parse_do_while_statement - Parse do-while loop
 * @parser: Parser instance
 *
 * Return: Do-while AST node, or NULL on error
 */
static ASTNode *parse_do_while_statement(Parser *parser)
{
	ASTNode *node, *body, *condition;

	advance(parser); /* consume 'do' */
	skip_whitespace(parser);

	body = parse_statement(parser);

	skip_whitespace(parser);

	if (!expect(parser, TOK_WHILE))
	{
		ast_node_destroy(body);
		return (NULL);
	}

	skip_whitespace(parser);

	if (!expect(parser, TOK_LPAREN))
	{
		ast_node_destroy(body);
		return (NULL);
	}

	condition = parse_expression(parser);
	skip_whitespace(parser);

	if (!expect(parser, TOK_RPAREN))
	{
		ast_node_destroy(body);
		ast_node_destroy(condition);
		return (NULL);
	}

	skip_whitespace(parser);
	expect(parser, TOK_SEMICOLON);

	node = ast_node_create(NODE_DO_WHILE, NULL);
	if (body)
		ast_node_add_child(node, body);
	if (condition)
		ast_node_add_child(node, condition);

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

	if (token->type == TOK_SWITCH)
		return (parse_switch_statement(parser));

	if (token->type == TOK_DO)
		return (parse_do_while_statement(parser));

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

	/* Variable declaration (built-in types) */
	if (is_type_keyword(token->type))
		return (parse_var_declaration(parser));

	/* Variable declaration (typedef'd types from symbol table) */
	if (token->type == TOK_IDENTIFIER &&
	    symbol_is_typedef(parser->symbols, token->lexeme))
		return (parse_var_declaration(parser));

	/* Variable declaration (heuristic: unknown type from headers) */
	/* Pattern: Identifier *var; or Identifier *var, ... */
	if (token->type == TOK_IDENTIFIER && looks_like_ptr_declaration(parser))
		return (parse_var_declaration(parser));

	/* Expression statement */
	node = ast_node_create(NODE_EXPR_STMT, NULL);
	if (node)
	{
		ASTNode *expr = parse_expression(parser);

		if (expr)
			ast_node_add_child(node, expr);
		skip_whitespace(parser);
		if (!expect(parser, TOK_SEMICOLON))
		{
			/* Error recovery: skip to next semicolon or brace */
			sync_to_semicolon(parser);
		}
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
	ASTNode *member;

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

		/* Parse struct members as variable declarations */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			skip_whitespace(parser);
			if (match(parser, TOK_RBRACE))
				break;

			/* Parse member as variable declaration */
			member = parse_var_declaration(parser);
			if (member)
				ast_node_add_child(node, member);
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
	ASTNode *enum_val;

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

		/* Parse enum values */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			skip_whitespace(parser);
			if (match(parser, TOK_RBRACE))
				break;

			/* Each enum value is an identifier */
			if (match(parser, TOK_IDENTIFIER))
			{
				enum_val = ast_node_create(NODE_ENUM_VALUE, peek(parser));
				advance(parser);
				skip_whitespace(parser);

				/* Check for = value */
				if (match(parser, TOK_ASSIGN))
				{
					advance(parser);
					skip_whitespace(parser);
					/* Consume the value expression */
					while (!is_at_end(parser) && !match(parser, TOK_COMMA) && !match(parser, TOK_RBRACE))
					{
						/* Store value as child if it's a literal */
						if (enum_val->child_count == 0 && (match(parser, TOK_INTEGER) || match(parser, TOK_IDENTIFIER)))
						{
							ast_node_add_child(enum_val, ast_node_create(NODE_LITERAL, peek(parser)));
						}
						advance(parser);
						skip_whitespace(parser);
					}
				}

				ast_node_add_child(node, enum_val);
			}

			/* Skip comma if present */
			if (match(parser, TOK_COMMA))
			{
				advance(parser);
				skip_whitespace(parser);
			}
		}

		expect(parser, TOK_RBRACE);
	}

	return (node);
}

/*
 * parse_union_definition - Parse union definition
 */
static ASTNode *parse_union_definition(Parser *parser)
{
	Token *name_token = NULL;
	ASTNode *node;

	advance(parser); /* consume 'union' */
	skip_whitespace(parser);

	/* Union name is optional */
	if (match(parser, TOK_IDENTIFIER))
	{
		name_token = peek(parser);
		advance(parser);
		skip_whitespace(parser);
	}

	node = ast_node_create(NODE_STRUCT, name_token); /* Reuse STRUCT node type */

	/* Parse union body if present */
	if (match(parser, TOK_LBRACE))
	{
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse union members (simplified - just consume until semicolon) */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			skip_whitespace(parser);
			if (match(parser, TOK_RBRACE))
				break;

			/* Parse member declaration */
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
	else if (match(parser, TOK_UNION))
	{
		inner = parse_union_definition(parser);
		if (inner)
			ast_node_add_child(node, inner);
		skip_whitespace(parser);
		/* Get the typedef alias name after the union */
		if (match(parser, TOK_IDENTIFIER))
		{
			node->token = peek(parser);
			advance(parser);
		}
	}
	else
	{
		/* Regular typedef - store base type tokens */
		Token **base_tokens = malloc(sizeof(Token *) * 16);
		int base_count = 0;
		int base_capacity = 16;
		TypedefData *td_data;

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
				/* Otherwise it's part of the type */
				if (base_count >= base_capacity)
				{
					base_capacity *= 2;
					base_tokens = realloc(base_tokens, sizeof(Token *) * base_capacity);
				}
				base_tokens[base_count++] = alias_token;
			}
			else
			{
				/* Store other tokens as part of base type */
				if (base_count >= base_capacity)
				{
					base_capacity *= 2;
					base_tokens = realloc(base_tokens, sizeof(Token *) * base_capacity);
				}
				base_tokens[base_count++] = peek(parser);
				advance(parser);
				skip_whitespace(parser);
			}
		}

		/* Store typedef data */
		td_data = malloc(sizeof(TypedefData));
		td_data->base_type_tokens = base_tokens;
		td_data->base_type_count = base_count;
		node->data = td_data;
	}

	skip_whitespace(parser);
	expect(parser, TOK_SEMICOLON);

	/* Register the typedef name in symbol table */
	if (node->token && node->token->lexeme)
		symbol_add(parser->symbols, node->token->lexeme, SYM_TYPEDEF);

	return (node);
}

/*
 * parse_parameter - Parse a single function parameter
 * @parser: Parser instance
 *
 * Return: Parameter AST node, or NULL
 */
static ASTNode *parse_parameter(Parser *parser)
{
	ASTNode *param;
	Token *type_start;
	Token **type_tokens = NULL;
	int type_count = 0;
	int type_capacity = 4;
	Token *name = NULL;

	type_tokens = malloc(sizeof(Token *) * type_capacity);
	if (!type_tokens)
		return (NULL);

	skip_whitespace(parser);
	type_start = peek(parser);
	if (!type_start)
	{
		free(type_tokens);
		return (NULL);
	}

	/* Collect type tokens (const, unsigned, int, *, etc.) */
	while (!is_at_end(parser) && !match(parser, TOK_COMMA) &&
	       !match(parser, TOK_RPAREN))
	{
		Token *tok = peek(parser);

		if (!tok)
			break;

		/* Check if this looks like a parameter name (identifier not followed by *) */
		if (tok->type == TOK_IDENTIFIER)
		{
			Token *next = peek_ahead(parser, 1);

			if (!next || next->type == TOK_COMMA ||
			    next->type == TOK_RPAREN ||
			    next->type == TOK_LBRACKET)
			{
				/* This is the parameter name */
				name = advance(parser);
				skip_whitespace(parser);
				/* Handle array parameters like argv[] */
				while (match(parser, TOK_LBRACKET))
				{
					if (type_count >= type_capacity)
					{
						type_capacity *= 2;
						type_tokens = realloc(type_tokens,
							sizeof(Token *) * type_capacity);
					}
					type_tokens[type_count++] = advance(parser);
					skip_whitespace(parser);
					if (match(parser, TOK_RBRACKET))
					{
						if (type_count >= type_capacity)
						{
							type_capacity *= 2;
							type_tokens = realloc(type_tokens,
								sizeof(Token *) * type_capacity);
						}
						type_tokens[type_count++] = advance(parser);
					}
					skip_whitespace(parser);
				}
				break;
			}
		}

		/* Add to type tokens */
		if (type_count >= type_capacity)
		{
			type_capacity *= 2;
			type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
		}
		type_tokens[type_count++] = advance(parser);
		skip_whitespace(parser);
	}

	if (type_count == 0 && !name)
	{
		free(type_tokens);
		return (NULL);
	}

	param = ast_node_create(NODE_PARAM, name);
	if (!param)
	{
		free(type_tokens);
		return (NULL);
	}

	/* Store type tokens in data field */
	{
		FunctionData *pdata = malloc(sizeof(FunctionData));

		if (pdata)
		{
			pdata->return_type_tokens = type_tokens;
			pdata->return_type_count = type_count;
			pdata->params = NULL;
			pdata->param_count = 0;
			param->data = pdata;
		}
		else
		{
			free(type_tokens);
		}
	}

	return (param);
}

/*
 * parse_function - Parse a function declaration
 * @parser: Parser instance
 *
 * Return: Function AST node, or NULL on error
 */
static ASTNode *parse_function(Parser *parser)
{
	Token *name;
	ASTNode *func, *body;
	int start_pos;
	Token **return_type_tokens = NULL;
	int return_type_count = 0;
	int return_type_capacity = 4;
	FunctionData *func_data;
	ASTNode **params = NULL;
	int param_count = 0;
	int param_capacity = 4;

	skip_whitespace(parser);
	start_pos = parser->current;

	return_type_tokens = malloc(sizeof(Token *) * return_type_capacity);
	if (!return_type_tokens)
		return (NULL);

	/* Collect return type tokens */
	/* Handle type modifiers: unsigned, signed, static, const, etc. */
	while (peek(parser) && (peek(parser)->type == TOK_UNSIGNED ||
	       peek(parser)->type == TOK_SIGNED ||
	       peek(parser)->type == TOK_STATIC ||
	       peek(parser)->type == TOK_CONST))
	{
		if (return_type_count >= return_type_capacity)
		{
			return_type_capacity *= 2;
			return_type_tokens = realloc(return_type_tokens,
				sizeof(Token *) * return_type_capacity);
		}
		return_type_tokens[return_type_count++] = advance(parser);
		skip_whitespace(parser);
	}

	/* Accept base type keywords OR identifiers (for typedef'd types like node_t) */
	if (peek(parser) && (peek(parser)->type == TOK_INT ||
	    peek(parser)->type == TOK_VOID ||
	    peek(parser)->type == TOK_CHAR_KW ||
	    peek(parser)->type == TOK_LONG ||
	    peek(parser)->type == TOK_SHORT ||
	    peek(parser)->type == TOK_FLOAT_KW ||
	    peek(parser)->type == TOK_DOUBLE ||
	    peek(parser)->type == TOK_STRUCT ||
	    peek(parser)->type == TOK_ENUM ||
	    peek(parser)->type == TOK_IDENTIFIER))
	{
		if (return_type_count >= return_type_capacity)
		{
			return_type_capacity *= 2;
			return_type_tokens = realloc(return_type_tokens,
				sizeof(Token *) * return_type_capacity);
		}
		return_type_tokens[return_type_count++] = advance(parser);
	}
	else
	{
		free(return_type_tokens);
		parser->current = start_pos;
		return (NULL);
	}

	skip_whitespace(parser);

	/* Handle multi-word types like "long long", "long int", etc. */
	while (peek(parser) && (peek(parser)->type == TOK_LONG ||
	       peek(parser)->type == TOK_INT ||
	       peek(parser)->type == TOK_DOUBLE))
	{
		if (return_type_count >= return_type_capacity)
		{
			return_type_capacity *= 2;
			return_type_tokens = realloc(return_type_tokens,
				sizeof(Token *) * return_type_capacity);
		}
		return_type_tokens[return_type_count++] = advance(parser);
		skip_whitespace(parser);
	}

	/* Handle struct/enum type names */
	if (return_type_count > 0 &&
	    (return_type_tokens[return_type_count - 1]->type == TOK_STRUCT ||
	     return_type_tokens[return_type_count - 1]->type == TOK_ENUM))
	{
		if (match(parser, TOK_IDENTIFIER))
		{
			if (return_type_count >= return_type_capacity)
			{
				return_type_capacity *= 2;
				return_type_tokens = realloc(return_type_tokens,
					sizeof(Token *) * return_type_capacity);
			}
			return_type_tokens[return_type_count++] = advance(parser);
			skip_whitespace(parser);
		}
	}

	/* Handle pointer types */
	while (match(parser, TOK_STAR))
	{
		if (return_type_count >= return_type_capacity)
		{
			return_type_capacity *= 2;
			return_type_tokens = realloc(return_type_tokens,
				sizeof(Token *) * return_type_capacity);
		}
		return_type_tokens[return_type_count++] = advance(parser);
		skip_whitespace(parser);
	}

	/* Check for function name */
	if (!match(parser, TOK_IDENTIFIER))
	{
		free(return_type_tokens);
		parser->current = start_pos;
		return (NULL);
	}

	name = advance(parser);
	skip_whitespace(parser);

	/* Check for opening parenthesis - if not present, it's not a function */
	if (!match(parser, TOK_LPAREN))
	{
		free(return_type_tokens);
		parser->current = start_pos;
		return (NULL);
	}

	func = ast_node_create(NODE_FUNCTION, name);
	if (!func)
	{
		free(return_type_tokens);
		parser->current = start_pos;
		return (NULL);
	}

	advance(parser); /* consume ( */
	skip_whitespace(parser);

	/* Parse parameters */
	params = malloc(sizeof(ASTNode *) * param_capacity);
	if (!params)
	{
		free(return_type_tokens);
		ast_node_destroy(func);
		parser->current = start_pos;
		return (NULL);
	}

	while (!is_at_end(parser) && !match(parser, TOK_RPAREN))
	{
		ASTNode *param = parse_parameter(parser);

		if (param)
		{
			if (param_count >= param_capacity)
			{
				param_capacity *= 2;
				params = realloc(params, sizeof(ASTNode *) * param_capacity);
			}
			params[param_count++] = param;
		}

		skip_whitespace(parser);
		if (match(parser, TOK_COMMA))
		{
			advance(parser);
			skip_whitespace(parser);
		}
	}

	if (!match(parser, TOK_RPAREN))
	{
		free(return_type_tokens);
		free(params);
		ast_node_destroy(func);
		parser->current = start_pos;
		return (NULL);
	}
	advance(parser); /* consume ) */

	skip_whitespace(parser);

	/* Store function signature data */
	func_data = malloc(sizeof(FunctionData));
	if (func_data)
	{
		func_data->return_type_tokens = return_type_tokens;
		func_data->return_type_count = return_type_count;
		func_data->params = params;
		func_data->param_count = param_count;
		func->data = func_data;
	}
	else
	{
		free(return_type_tokens);
		free(params);
	}

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

		/* Try to parse struct - but check if it's a definition or function return type */
		if (match(parser, TOK_STRUCT))
		{
			/* Look ahead to determine if this is a struct definition or function */
			Token *next1 = peek_ahead(parser, 1); /* struct name or { */
			Token *next2 = peek_ahead(parser, 2); /* { or * or identifier */

			/* struct { ... } is anonymous struct def */
			/* struct name { ... } is named struct def */
			/* struct name *func() or struct name func() is function */
			if ((next1 && next1->type == TOK_LBRACE) ||
			    (next2 && next2->type == TOK_LBRACE))
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
			/* Otherwise fall through to function parsing */
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

		/* Try to parse union */
		if (match(parser, TOK_UNION))
		{
			func = parse_union_definition(parser);
			if (func)
			{
				ast_node_add_child(program, func);
				skip_whitespace(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
			continue;
		}

		/* Try to parse a function first */
		func = parse_function(parser);
		if (func)
		{
			ast_node_add_child(program, func);
		}
		else
		{
			/* Not a function - try parsing as global variable declaration */
			Token *tok = peek(parser);

			if (tok && (is_type_keyword(tok->type) ||
			    (tok->type == TOK_IDENTIFIER &&
			     symbol_is_typedef(parser->symbols, tok->lexeme))))
			{
				func = parse_var_declaration(parser);
				if (func)
				{
					ast_node_add_child(program, func);
					continue;
				}
			}

			/* If parsing failed, skip to next top-level declaration */
			/* Don't break on errors - try to continue parsing */

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
