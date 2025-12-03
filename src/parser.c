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
static int skip_whitespace(Parser *parser);
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
static int is_attribute_node(ASTNode *node);
static int merge_attribute_with_previous(ASTNode *parent, ASTNode *node);
static void add_child_handling_attribute(ASTNode *parent, ASTNode *child);
static void skip_attribute(Parser *parser);

/*
 * parser_create - Create a new parser
 * @tokens: Array of tokens
 * @token_count: Number of tokens
 * @source: Original source code (for preserving unparseable constructs)
 *
 * Return: Pointer to new parser, or NULL on failure
 */
Parser *parser_create(Token **tokens, int token_count, const char *source)
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
	parser->source = source;

	/* Add common C library typedefs */
	if (parser->symbols)
	{
		symbol_add(parser->symbols, "size_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "ssize_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "ptrdiff_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "intptr_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "uintptr_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "int8_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "int16_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "int32_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "int64_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "uint8_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "uint16_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "uint32_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "uint64_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "va_list", SYM_TYPEDEF);
		symbol_add(parser->symbols, "FILE", SYM_TYPEDEF);
		symbol_add(parser->symbols, "DIR", SYM_TYPEDEF);
		symbol_add(parser->symbols, "time_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "clock_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "pid_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "uid_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "gid_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "off_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "mode_t", SYM_TYPEDEF);
		symbol_add(parser->symbols, "bool", SYM_TYPEDEF);
	}

	/* Initialize comment buffer */
	parser->pending_comments = NULL;
	parser->pending_comment_count = 0;
	parser->pending_comment_capacity = 0;
	parser->last_token_line = 0;

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

	free(parser->pending_comments);
	free(parser);
}

/*
 * parser_had_errors - Check if parser encountered errors
 * @parser: Parser instance
 *
 * Return: 1 if errors occurred, 0 otherwise
 */
int parser_had_errors(Parser *parser)
{
	if (!parser)
		return (1);
	return (parser->error_count > 0);
}

/*
 * create_raw_text_node - Create a raw text node from token range
 * @parser: Parser instance
 * @start_idx: Starting token index
 * @end_idx: Ending token index (exclusive)
 *
 * Return: Raw text node, or NULL on error
 */
static ASTNode *create_raw_text_node(Parser *parser, int start_idx, int end_idx)
{
	ASTNode *node;
	RawTextData *data;
	size_t total_len = 0;
	char *text;
	char *p;
	int i;

	if (start_idx >= end_idx || start_idx < 0 || end_idx > parser->token_count)
		return (NULL);

	/* Calculate total length needed - just sum all token lexemes */
	for (i = start_idx; i < end_idx; i++)
	{
		Token *tok = parser->tokens[i];

		if (tok->lexeme)
			total_len += strlen(tok->lexeme);
	}

	text = malloc(total_len + 1);
	if (!text)
		return (NULL);

	/* Build the text - concatenate all token lexemes including whitespace */
	p = text;
	for (i = start_idx; i < end_idx; i++)
	{
		Token *tok = parser->tokens[i];
		size_t len;

		if (tok->lexeme)
		{
			len = strlen(tok->lexeme);
			memcpy(p, tok->lexeme, len);
			p += len;
		}
	}
	*p = '\0';

	node = ast_node_create(NODE_RAW_TEXT, parser->tokens[start_idx]);
	if (!node)
	{
		free(text);
		return (NULL);
	}

	data = malloc(sizeof(RawTextData));
	if (!data)
	{
		free(text);
		ast_node_destroy(node);
		return (NULL);
	}

	data->text = text;
	data->length = (int)(p - text);
	node->data = data;

	return (node);
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
 * looks_like_ptr_declaration - Check if tokens look like "Type *var" or "Type **var"
 * @parser: Parser instance
 *
 * Heuristic: IDENTIFIER STAR+ IDENTIFIER followed by ; or , or = or [
 * Return: 1 if looks like declaration, 0 otherwise
 */
static int looks_like_ptr_declaration(Parser *parser)
{
	Token *t0 = peek_ahead(parser, 0);
	Token *t1 = peek_ahead(parser, 1);
	Token *t2 = peek_ahead(parser, 2);
	Token *t3 = peek_ahead(parser, 3);
	Token *t4 = peek_ahead(parser, 4);

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

	/* Pattern: IDENTIFIER ** IDENTIFIER (;|,|=|[) */
	if (t0->type == TOK_IDENTIFIER &&
	    t1->type == TOK_STAR &&
	    t2->type == TOK_STAR &&
	    t3 && t3->type == TOK_IDENTIFIER &&
	    t4 && (t4->type == TOK_SEMICOLON || t4->type == TOK_COMMA ||
		   t4->type == TOK_ASSIGN || t4->type == TOK_LBRACKET))
	{
		return (1);
	}

	/* Pattern: IDENTIFIER *** IDENTIFIER (;|,|=|[) - triple pointer */
	if (t0->type == TOK_IDENTIFIER &&
	    t1->type == TOK_STAR &&
	    t2->type == TOK_STAR)
	{
		Token *t5 = peek_ahead(parser, 5);

		if (t3 && t3->type == TOK_STAR &&
		    t4 && t4->type == TOK_IDENTIFIER &&
		    t5 && (t5->type == TOK_SEMICOLON || t5->type == TOK_COMMA ||
			   t5->type == TOK_ASSIGN || t5->type == TOK_LBRACKET))
		{
			return (1);
		}
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
	Token *token;

	if (is_at_end(parser))
		return (NULL);
	token = parser->tokens[parser->current++];
	/* Track line of last significant token for trailing comments */
	if (token && token->type != TOK_WHITESPACE && token->type != TOK_NEWLINE)
		parser->last_token_line = token->line;
	return (token);
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
		/* Silently track error - file will be left unchanged */
		parser->error_count++;
		return (NULL);
	}

	return (advance(parser));
}

/*
 * add_pending_comment - Add a comment to the pending buffer
 * @parser: Parser instance
 * @comment: Comment token to add
 */
static void add_pending_comment(Parser *parser, Token *comment)
{
	if (parser->pending_comment_count >= parser->pending_comment_capacity)
	{
		int new_cap = parser->pending_comment_capacity == 0
			? 4 : parser->pending_comment_capacity * 2;
		Token **new_buf = realloc(parser->pending_comments,
			sizeof(Token *) * new_cap);
		if (!new_buf)
			return;
		parser->pending_comments = new_buf;
		parser->pending_comment_capacity = new_cap;
	}
	parser->pending_comments[parser->pending_comment_count++] = comment;
}

/*
 * attach_pending_comments - Attach pending comments to a node as leading
 * @parser: Parser instance
 * @node: Node to attach comments to
 */
static void attach_pending_comments(Parser *parser, ASTNode *node)
{
	int i;

	if (!node || parser->pending_comment_count == 0)
		return;

	for (i = 0; i < parser->pending_comment_count; i++)
		ast_node_add_leading_comment(node, parser->pending_comments[i]);

	parser->pending_comment_count = 0;
}

/*
 * collect_trailing_comments - Collect comments on same line as trailing
 * @parser: Parser instance
 * @node: Node to attach trailing comments to
 *
 * Collects any comments that appear on the same line as the last token,
 * before any newline. These are attached as trailing comments.
 */
static void collect_trailing_comments(Parser *parser, ASTNode *node)
{
	Token *token;

	if (!node)
		return;

	/* Skip only horizontal whitespace, collect comments on same line */
	while (!is_at_end(parser))
	{
		token = peek(parser);
		if (token->type == TOK_WHITESPACE)
		{
			advance(parser);
		}
		else if ((token->type == TOK_COMMENT_LINE ||
			  token->type == TOK_COMMENT_BLOCK) &&
			 token->line == parser->last_token_line)
		{
			/* Comment on same line - it's a trailing comment */
			ast_node_add_trailing_comment(node, token);
			advance(parser);
		}
		else
		{
			/* Newline or other token - stop */
			break;
		}
	}
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
 * skip_whitespace - Skip whitespace, newlines, and collect comments
 * @parser: Parser instance
 *
 * Comments are added to parser->pending_comments to be attached to the next node.
 * Returns the number of blank lines encountered (empty lines, not comment lines).
 */
static int skip_whitespace(Parser *parser)
{
	Token *token;
	int newline_count = 0;
	int last_was_comment = 0;
	int blank_lines = 0;

	while (!is_at_end(parser))
	{
		token = peek(parser);
		if (token->type == TOK_WHITESPACE)
		{
			advance(parser);
		}
		else if (token->type == TOK_NEWLINE)
		{
			newline_count++;
			/* Count consecutive newlines beyond the first as blank lines */
			/* But reset if we just had a comment (comment line doesn't count) */
			if (newline_count > 1 && !last_was_comment)
				blank_lines++;
			last_was_comment = 0;
			advance(parser);
		}
		else if (token->type == TOK_COMMENT_LINE ||
			 token->type == TOK_COMMENT_BLOCK)
		{
			add_pending_comment(parser, token);
			last_was_comment = 1;
			newline_count = 0; /* Reset - comment consumes the newline */
			advance(parser);
		}
		else
			break;
	}

	return (blank_lines);
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
		type == TOK_ENUM || type == TOK_UNION ||
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

	/* Type expression (for macro args like va_arg(ap, int) or va_arg(ap, char *)) */
	if (is_type_keyword(token->type) ||
	    token->type == TOK_STRUCT || token->type == TOK_ENUM ||
	    token->type == TOK_UNION)
	{
		Token **type_tokens = NULL;
		int type_count = 0;
		int type_capacity = 4;
		FunctionData *type_data;

		type_tokens = malloc(sizeof(Token *) * type_capacity);
		if (!type_tokens)
			return (NULL);

		/* Collect type tokens until comma or rparen */
		while (!is_at_end(parser) && !match(parser, TOK_COMMA) &&
		       !match(parser, TOK_RPAREN))
		{
			token = peek(parser);
			if (!token)
				break;

			if (type_count >= type_capacity)
			{
				type_capacity *= 2;
				type_tokens = realloc(type_tokens,
					sizeof(Token *) * type_capacity);
			}
			type_tokens[type_count++] = advance(parser);
			skip_whitespace(parser);
		}

		node = ast_node_create(NODE_TYPE_EXPR, type_tokens[0]);
		if (!node)
		{
			free(type_tokens);
			return (NULL);
		}

		/* Store type tokens in data */
		type_data = malloc(sizeof(FunctionData));
		if (type_data)
		{
			type_data->return_type_tokens = type_tokens;
			type_data->return_type_count = type_count;
			type_data->params = NULL;
			type_data->param_count = 0;
			node->data = type_data;
		}
		else
		{
			free(type_tokens);
		}

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
				else
				{
					/* Failed to parse - skip to comma or ) */
					/* This handles cases like va_arg(ap, int) */
					while (!is_at_end(parser) &&
					       !match(parser, TOK_COMMA) &&
					       !match(parser, TOK_RPAREN))
						advance(parser);
				}
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
			CastData *cast_data = malloc(sizeof(CastData));
			int type_capacity = 8;

			if (cast_data)
			{
				cast_data->type_tokens = malloc(sizeof(Token *) * type_capacity);
				cast_data->type_count = 0;

				/* Collect all type tokens */
				while (!is_at_end(parser))
				{
					token = peek(parser);
					if (!token || token->type == TOK_RPAREN)
						break;

					if (cast_data->type_count >= type_capacity)
					{
						type_capacity *= 2;
						cast_data->type_tokens = realloc(cast_data->type_tokens,
							sizeof(Token *) * type_capacity);
					}
					cast_data->type_tokens[cast_data->type_count++] = token;
					advance(parser);
					skip_whitespace(parser);
				}
				cast_node->data = cast_data;
			}

			expect(parser, TOK_RPAREN);
			skip_whitespace(parser);

			/* Parse the expression being cast */
			node = parse_unary(parser);
			if (node)
				ast_node_add_child(cast_node, node);
			return (cast_node);
		}

		/* Regular parenthesized expression - wrap to preserve parens */
		node = parse_expression(parser);
		skip_whitespace(parser);
		expect(parser, TOK_RPAREN);

		if (node)
		{
			ASTNode *paren_node = ast_node_create(NODE_PAREN, NULL);

			if (paren_node)
			{
				ast_node_add_child(paren_node, node);
				return (paren_node);
			}
		}
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
				else
				{
					/* Failed to parse - skip to comma or ) */
					/* This handles cases like va_arg(ap, int) */
					while (!is_at_end(parser) &&
					       !match(parser, TOK_COMMA) &&
					       !match(parser, TOK_RPAREN))
						advance(parser);
				}
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
			/* Mark as postfix by setting data to non-NULL */
			postfix->data = (void *)1;
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
			Token *lookahead;

			advance(parser);
			skip_whitespace(parser);
			next = peek(parser);

			/* Look ahead to determine if this is a type or expression */
			/* It's a type if: it's a type keyword, a known typedef, or
			 * an identifier followed by * or ) (likely a type name) */
			lookahead = NULL;
			if (next && next->type == TOK_IDENTIFIER)
			{
				int saved_pos = parser->current;

				advance(parser);
				skip_whitespace(parser);
				lookahead = peek(parser);
				parser->current = saved_pos;
			}

			/* Check if it's sizeof(type) */
			if (next && (is_type_keyword(next->type) ||
			    (next->type == TOK_IDENTIFIER &&
			     symbol_is_typedef(parser->symbols, next->lexeme)) ||
			    (next->type == TOK_IDENTIFIER && lookahead &&
			     (lookahead->type == TOK_STAR ||
			      lookahead->type == TOK_RPAREN))))
			{
				/* Collect all type tokens */
				SizeofData *sizeof_data = malloc(sizeof(SizeofData));
				Token **type_tokens = malloc(sizeof(Token *) * 16);
				int type_count = 0;
				int type_capacity = 16;

				while (!is_at_end(parser) && !match(parser, TOK_RPAREN))
				{
					Token *tok = peek(parser);

					if (!tok)
						break;

					/* Skip whitespace tokens but collect everything else */
					if (tok->type != TOK_WHITESPACE &&
					    tok->type != TOK_NEWLINE)
					{
						if (type_count >= type_capacity)
						{
							type_capacity *= 2;
							type_tokens = realloc(type_tokens,
								sizeof(Token *) * type_capacity);
						}
						type_tokens[type_count++] = tok;
					}
					advance(parser);
					skip_whitespace(parser);
				}
				expect(parser, TOK_RPAREN);

				sizeof_data->type_tokens = type_tokens;
				sizeof_data->type_count = type_count;
				node->data = sizeof_data;
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
 * parse_initializer - Parse an initializer (expression or brace-enclosed list)
 */
static ASTNode *parse_initializer(Parser *parser)
{
	ASTNode *init;

	skip_whitespace(parser);

	/* Check for brace-enclosed initializer list: {1, 2, 3} */
	if (match(parser, TOK_LBRACE))
	{
		init = ast_node_create(NODE_INIT_LIST, peek(parser));
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse initializer elements */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			ASTNode *elem = parse_initializer(parser); /* recursive for nested */

			if (elem)
				ast_node_add_child(init, elem);

			skip_whitespace(parser);
			if (match(parser, TOK_COMMA))
			{
				advance(parser);
				skip_whitespace(parser);
			}
		}

		if (match(parser, TOK_RBRACE))
			advance(parser);

		return (init);
	}

	/* Regular expression */
	return (parse_expression(parser));
}

/*
 * parse_func_ptr_decl - Parse function pointer declaration
 * Handles: int (*callback)(int, int);
 */
static ASTNode *parse_func_ptr_decl(Parser *parser, Token **type_tokens,
				    int type_count)
{
	ASTNode *node;
	FuncPtrData *fp_data;
	Token **param_tokens = NULL;
	int param_count = 0;
	int param_capacity = 16;
	Token *name_token = NULL;
	int paren_depth;

	/* We're at '(' - expect '(' '*' IDENTIFIER ')' '(' params ')' */
	if (!match(parser, TOK_LPAREN))
		return (NULL);

	advance(parser); /* consume '(' */
	skip_whitespace(parser);

	/* Expect '*' */
	if (!match(parser, TOK_STAR))
		return (NULL);
	advance(parser); /* consume '*' */
	skip_whitespace(parser);

	/* Expect identifier (function pointer name) */
	name_token = expect(parser, TOK_IDENTIFIER);
	if (!name_token)
		return (NULL);
	skip_whitespace(parser);

	/* Expect ')' */
	if (!match(parser, TOK_RPAREN))
		return (NULL);
	advance(parser); /* consume ')' */
	skip_whitespace(parser);

	/* Expect '(' for parameters */
	if (!match(parser, TOK_LPAREN))
		return (NULL);
	advance(parser); /* consume '(' */
	skip_whitespace(parser);

	/* Collect parameter tokens until matching ')' */
	param_tokens = malloc(sizeof(Token *) * param_capacity);
	paren_depth = 1;

	while (!is_at_end(parser) && paren_depth > 0)
	{
		Token *tok = peek(parser);

		if (tok->type == TOK_LPAREN)
			paren_depth++;
		else if (tok->type == TOK_RPAREN)
		{
			paren_depth--;
			if (paren_depth == 0)
				break;
		}

		if (param_count >= param_capacity)
		{
			param_capacity *= 2;
			param_tokens = realloc(param_tokens,
					       sizeof(Token *) * param_capacity);
		}
		param_tokens[param_count++] = advance(parser);
		skip_whitespace(parser);
	}

	/* Consume closing ')' */
	if (match(parser, TOK_RPAREN))
		advance(parser);

	node = ast_node_create(NODE_FUNC_PTR, type_tokens[0]);

	fp_data = malloc(sizeof(FuncPtrData));
	if (fp_data)
	{
		fp_data->return_type_tokens = type_tokens;
		fp_data->return_type_count = type_count;
		fp_data->name_token = name_token;
		fp_data->param_tokens = param_tokens;
		fp_data->param_count = param_count;
		node->data = fp_data;
	}
	else
	{
		free(param_tokens);
	}

	return (node);
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

		/* Handle struct/enum/union name (e.g., "struct node" or "enum color") */
		if ((type_token->type == TOK_STRUCT || type_token->type == TOK_ENUM ||
		     type_token->type == TOK_UNION) &&
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

			/* Handle struct/enum/union name after type keyword */
			if ((type_tokens[type_count - 1]->type == TOK_STRUCT ||
			     type_tokens[type_count - 1]->type == TOK_ENUM ||
			     type_tokens[type_count - 1]->type == TOK_UNION) &&
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
	/* Also handle const/volatile after pointer: char * const ptr */
	while (match(parser, TOK_STAR) || match(parser, TOK_CONST) ||
	       match(parser, TOK_VOLATILE))
	{
		if (type_count >= type_capacity)
		{
			type_capacity *= 2;
			type_tokens = realloc(type_tokens, sizeof(Token *) * type_capacity);
		}
		type_tokens[type_count++] = advance(parser);
		skip_whitespace(parser);
	}

	/* Check for function pointer: int (*fn)(int, int) */
	if (match(parser, TOK_LPAREN))
	{
		/* Look ahead for (*name) pattern */
		int saved_pos = parser->current;
		Token *t1 = peek(parser);

		if (t1 && t1->type == TOK_LPAREN)
		{
			parser->current++;
			skip_whitespace(parser);
			Token *t2 = peek(parser);

			if (t2 && t2->type == TOK_STAR)
			{
				ASTNode *fp_node;

				/* This looks like a function pointer */
				parser->current = saved_pos;
				fp_node = parse_func_ptr_decl(parser, type_tokens,
							     type_count);
				/* Consume the semicolon after func ptr decl */
				skip_whitespace(parser);
				expect(parser, TOK_SEMICOLON);
				return (fp_node);
			}
		}
		parser->current = saved_pos;
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
		skip_whitespace(parser);
		ASTNode *init = parse_initializer(parser);

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

	/* Check for else - only consume whitespace if there might be an else */
	{
		int saved_pos = parser->current;
		int ws_count = skip_whitespace(parser);

		if (match(parser, TOK_ELSE))
		{
			advance(parser);
			skip_whitespace(parser);
			else_branch = parse_statement(parser);
			if (else_branch)
				ast_node_add_child(node, else_branch);
		}
		else
		{
			/* No else - restore position so blank lines are preserved */
			parser->current = saved_pos;
			(void)ws_count;
		}
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
static int is_attribute_node(ASTNode *node)
{
	const char *p;
	const char *end;

	if (!node || !node->source_start || !node->source_end)
		return (0);

	p = node->source_start;
	end = node->source_end;

	while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
		p++;

	if ((end - p) < 13)
		return (0);

	return (strncmp(p, "__attribute__", 13) == 0);
}

static int merge_attribute_with_previous(ASTNode *parent, ASTNode *node)
{
	ASTNode *prev;

	if (!parent || !node)
		return (0);
	if (!is_attribute_node(node))
		return (0);
	if (parent->child_count <= 0)
		return (0);

	prev = parent->children[parent->child_count - 1];
	if (!prev)
		return (0);

	if (node->source_end &&
	    (!prev->source_end || node->source_end > prev->source_end))
		prev->source_end = node->source_end;

	ast_node_destroy(node);
	return (1);
}

static void add_child_handling_attribute(ASTNode *parent, ASTNode *child)
{
	if (!parent || !child)
		return;

	if (merge_attribute_with_previous(parent, child))
		return;

	ast_node_add_child(parent, child);
}

/*
 * skip_attribute - Skip over __attribute__((...)) if present
 * @parser: Parser instance
 *
 * Consumes the __attribute__ token and its nested parentheses.
 * Used for function prototypes with attributes like:
 *   static char foo(int x) __attribute__((unused));
 */
static void skip_attribute(Parser *parser)
{
	Token *tok;
	int paren_depth;

	skip_whitespace(parser);
	tok = peek(parser);

	if (!tok || tok->type != TOK_IDENTIFIER)
		return;

	if (strcmp(tok->lexeme, "__attribute__") != 0)
		return;

	advance(parser); /* consume __attribute__ */
	skip_whitespace(parser);

	/* Expect (( */
	if (!match(parser, TOK_LPAREN))
		return;

	paren_depth = 0;

	/* Consume everything until matching )) */
	while (!is_at_end(parser))
	{
		tok = peek(parser);
		if (tok->type == TOK_LPAREN)
		{
			paren_depth++;
			advance(parser);
		}
		else if (tok->type == TOK_RPAREN)
		{
			paren_depth--;
			advance(parser);
			if (paren_depth <= 0)
				break;
		}
		else
		{
			advance(parser);
		}
	}

	skip_whitespace(parser);
}

static ASTNode *parse_block(Parser *parser)
{
	ASTNode *block, *stmt;
	int blank_lines;
	Token *open_brace;

	skip_whitespace(parser);

	open_brace = peek(parser);
	if (!expect(parser, TOK_LBRACE))
		return (NULL);

	block = ast_node_create(NODE_BLOCK, NULL);
	if (!block)
		return (NULL);

	/* Record source start from opening brace */
	if (parser->source && open_brace && open_brace->source_offset >= 0)
		block->source_start = parser->source + open_brace->source_offset;

	skip_whitespace(parser);

	/* Parse statements until we hit } */
	while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
	{
		blank_lines = skip_whitespace(parser);

		if (match(parser, TOK_RBRACE))
			break;

		stmt = parse_statement(parser);
		if (stmt)
		{
			attach_pending_comments(parser, stmt);
			/* Preserve user blank lines (max 1) */
			stmt->blank_lines_before = (blank_lines > 0 ? 1 : 0);
			add_child_handling_attribute(block, stmt);
		}
		/* Don't skip whitespace here - we do it at the start of the loop */
	}

	skip_whitespace(parser);
	{
		Token *close_brace = peek(parser);

		if (close_brace && parser->source && close_brace->source_offset >= 0)
			block->source_end = parser->source + close_brace->source_offset +
					    close_brace->length;
	}
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
	Token *start_token;
	ASTNode *node;
	/* Save pending comments before parsing - they belong to this statement */
	Token **saved_comments = NULL;
	int saved_count = 0;
	int i;
	int start_pos;

	skip_whitespace(parser);
	token = peek(parser);

	if (!token)
		return (NULL);

	/* Remember start position for source preservation */
	start_pos = parser->current;
	start_token = token;

	/* Save pending comments to attach after parsing */
	if (parser->pending_comment_count > 0)
	{
		saved_count = parser->pending_comment_count;
		saved_comments = malloc(sizeof(Token *) * saved_count);
		if (saved_comments)
		{
			for (i = 0; i < saved_count; i++)
				saved_comments[i] = parser->pending_comments[i];
		}
		parser->pending_comment_count = 0;
	}

	/* Control flow statements */
	if (token->type == TOK_IF)
		node = parse_if_statement(parser);
	else if (token->type == TOK_WHILE)
		node = parse_while_statement(parser);
	else if (token->type == TOK_FOR)
		node = parse_for_statement(parser);
	else if (token->type == TOK_SWITCH)
		node = parse_switch_statement(parser);
	else if (token->type == TOK_DO)
		node = parse_do_while_statement(parser);
	/* Return statement */
	else if (token->type == TOK_RETURN)
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
	}
	/* Break statement */
	else if (token->type == TOK_BREAK)
	{
		node = ast_node_create(NODE_BREAK, token);
		advance(parser);
		skip_whitespace(parser);
		expect(parser, TOK_SEMICOLON);
	}
	/* Continue statement */
	else if (token->type == TOK_CONTINUE)
	{
		node = ast_node_create(NODE_CONTINUE, token);
		advance(parser);
		skip_whitespace(parser);
		expect(parser, TOK_SEMICOLON);
	}
	/* Preprocessor directive */
	else if (token->type == TOK_PREPROCESSOR)
	{
		node = ast_node_create(NODE_PREPROCESSOR, token);
		advance(parser);
	}
	/* Typedef in function body (local typedef) */
	else if (token->type == TOK_TYPEDEF)
		node = parse_typedef(parser);
	/* Block statement */
	else if (token->type == TOK_LBRACE)
		node = parse_block(parser);
	/* Variable declaration (built-in types, but not typedef) */
	else if (is_type_keyword(token->type) && token->type != TOK_TYPEDEF)
		node = parse_var_declaration(parser);
	/* Variable declaration (typedef'd types from symbol table) */
	else if (token->type == TOK_IDENTIFIER &&
		 symbol_is_typedef(parser->symbols, token->lexeme))
		node = parse_var_declaration(parser);
	/* Variable declaration (heuristic: unknown type from headers) */
	/* Pattern: Identifier *var; or Identifier *var, ... */
	else if (token->type == TOK_IDENTIFIER && looks_like_ptr_declaration(parser))
		node = parse_var_declaration(parser);
	/* Expression statement */
	else
	{
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
	}

	/* Set source positions for safe formatting */
	if (node && parser->source && start_token->source_offset >= 0)
	{
		Token *end_token = NULL;
		int end_pos = parser->current - 1;

		if (end_pos >= 0 && end_pos < parser->token_count)
			end_token = parser->tokens[end_pos];

		node->source_start = parser->source + start_token->source_offset;
		if (end_token && end_token->source_offset >= 0)
			node->source_end = parser->source + end_token->source_offset +
					   end_token->length;
		else
			node->source_end = node->source_start;
	}

	/* Attach saved comments to the parsed node */
	if (saved_comments && node)
	{
		for (i = 0; i < saved_count; i++)
			ast_node_add_leading_comment(node, saved_comments[i]);
		free(saved_comments);
	}

	/* Collect any trailing comments on the same line */
	collect_trailing_comments(parser, node);

	(void)start_pos;  /* Silence unused warning */

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
			else
			{
				/* Skip to semicolon or closing brace to avoid infinite loop */
				while (!is_at_end(parser) && !match(parser, TOK_SEMICOLON) &&
				       !match(parser, TOK_RBRACE))
					advance(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
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

		/* Collect type tokens first */
		while (!is_at_end(parser) && is_type_keyword(peek(parser)->type))
		{
			if (base_count >= base_capacity)
			{
				base_capacity *= 2;
				base_tokens = realloc(base_tokens,
						      sizeof(Token *) * base_capacity);
			}
			base_tokens[base_count++] = advance(parser);
			skip_whitespace(parser);
		}

		/* Also collect pointer as part of return type: void * */
		while (match(parser, TOK_STAR))
		{
			if (base_count >= base_capacity)
			{
				base_capacity *= 2;
				base_tokens = realloc(base_tokens,
						      sizeof(Token *) * base_capacity);
			}
			base_tokens[base_count++] = advance(parser);
			skip_whitespace(parser);
		}

		/* Check for function pointer typedef: int (*name)(params) */
		if (match(parser, TOK_LPAREN))
		{
			int saved_pos = parser->current;

			advance(parser);
			skip_whitespace(parser);
			if (match(parser, TOK_STAR))
			{
				/* This is a function pointer typedef */
				ASTNode *fp_node;

				parser->current = saved_pos;
				fp_node = parse_func_ptr_decl(parser, base_tokens,
							      base_count);
				if (fp_node)
				{
					/* Register the typedef */
					FuncPtrData *fp_data = (FuncPtrData *)fp_node->data;

					if (fp_data && fp_data->name_token)
					{
						node->token = fp_data->name_token;
						symbol_add(parser->symbols,
							   fp_data->name_token->lexeme,
							   SYM_TYPEDEF);
					}
					/* Store the func ptr node as child */
					ast_node_add_child(node, fp_node);
					/* Skip semicolon */
					skip_whitespace(parser);
					expect(parser, TOK_SEMICOLON);
					return (node);
				}
			}
			parser->current = saved_pos;
		}

		/* Regular typedef processing */
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

	skip_whitespace(parser);

	/* Handle variadic parameter (...) */
	if (match(parser, TOK_ELLIPSIS))
	{
		Token *ellipsis = advance(parser);

		param = ast_node_create(NODE_PARAM, ellipsis);
		if (!param)
			return (NULL);
		param->data = NULL;
		return (param);
	}

	type_tokens = malloc(sizeof(Token *) * type_capacity);
	if (!type_tokens)
		return (NULL);

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

	/* Attach any pending comments to the function */
	attach_pending_comments(parser, func);

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

	/* Skip __attribute__((...)) if present before semicolon or body */
	skip_attribute(parser);

	/* Check if this is a prototype (ends with ;) or definition (has body) */
	if (match(parser, TOK_SEMICOLON))
	{
		/* Function prototype - no body */
		advance(parser);
		return (func);
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
	int blank_lines;

	program = ast_node_create(NODE_PROGRAM, NULL);
	if (!program)
		return (NULL);

	while (!is_at_end(parser))
	{
		blank_lines = skip_whitespace(parser);

		if (is_at_end(parser))
			break;

		/* Parse preprocessor directives */
		if (match(parser, TOK_PREPROCESSOR))
		{
			Token *pp_token = advance(parser);
			ASTNode *pp_node = ast_node_create(NODE_PREPROCESSOR, pp_token);
			if (pp_node)
			{
				attach_pending_comments(parser, pp_node);
				pp_node->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				add_child_handling_attribute(program, pp_node);
			}
			continue;
		}

		/* Try to parse typedef */
		if (match(parser, TOK_TYPEDEF))
		{
			func = parse_typedef(parser);
			if (func)
			{
				attach_pending_comments(parser, func);
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				add_child_handling_attribute(program, func);
			}
			continue;
		}

		/* Try to parse struct - but check if it's a definition or function return type */
		if (match(parser, TOK_STRUCT))
		{
			/* Look ahead to determine if this is a struct definition or function */
			Token *next1 = peek_ahead(parser, 1); /* struct name or { */
			Token *next2 = peek_ahead(parser, 2); /* { or * or identifier or ; */

			/* struct { ... } is anonymous struct def */
			/* struct name { ... } is named struct def */
			/* struct name; is forward declaration */
			/* struct name *func() or struct name func() is function */
			if ((next1 && next1->type == TOK_LBRACE) ||
			    (next2 && next2->type == TOK_LBRACE) ||
			    (next1 && next1->type == TOK_IDENTIFIER &&
			     next2 && next2->type == TOK_SEMICOLON))
			{
				func = parse_struct_definition(parser);
				if (func)
				{
					attach_pending_comments(parser, func);
					func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
					add_child_handling_attribute(program, func);
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
				attach_pending_comments(parser, func);
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				add_child_handling_attribute(program, func);
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
				attach_pending_comments(parser, func);
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				ast_node_add_child(program, func);
				skip_whitespace(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
			continue;
		}

		/* Save position before attempting to parse */
		{
			int saved_pos = parser->current;

			/* Try to parse a function first */
			func = parse_function(parser);
			if (func)
			{
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				ast_node_add_child(program, func);
			}
			else
			{
				/* Restore position to include all tokens in raw text */
				parser->current = saved_pos;

				/* Not a function - try parsing as global variable declaration */
				Token *tok = peek(parser);

				if (tok && (is_type_keyword(tok->type) ||
				    (tok->type == TOK_IDENTIFIER &&
				     symbol_is_typedef(parser->symbols, tok->lexeme))))
				{
					func = parse_var_declaration(parser);
					if (func)
					{
						attach_pending_comments(parser, func);
						func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
						ast_node_add_child(program, func);
						continue;
					}
					/* Restore again if var decl also failed */
					parser->current = saved_pos;
				}

				/* If parsing failed, create raw text node for this section */
				/* and skip to next top-level declaration */
				{
					int start_idx = parser->current;
					int end_idx;
					ASTNode *raw_node;

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

				end_idx = parser->current;

				/* Create raw text node to preserve the skipped content */
				raw_node = create_raw_text_node(parser, start_idx, end_idx);
				if (raw_node)
				{
					attach_pending_comments(parser, raw_node);
					raw_node->blank_lines_before = (blank_lines > 0 ? 1 : 0);
					ast_node_add_child(program, raw_node);
				}
			}
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
