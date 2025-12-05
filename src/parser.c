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
static ASTNode *create_unparsed_node(Parser *parser, int start_index, int end_index);
static ASTNode *recover_top_level(Parser *parser, int start_index);
static ASTNode *recover_statement(Parser *parser, int start_index);
static int looks_like_type_in_parens(Parser *parser, int start_index,
	int *closing_index);
static void skip_gnu_attributes(Parser *parser);
static void clear_pending_comments(Parser *parser);
static void add_unparsed_child(Parser *parser, ASTNode *parent, int start_index);
static char *copy_token_text(Parser *parser, int start_index, int end_index);
static int token_allowed_in_type(Token *token);
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
	parser->whitespace_start = 0;
	parser->symbols = symbol_table_create(NULL);

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
		symbol_add(parser->symbols, "RawSegmentData", SYM_TYPEDEF);
		symbol_add(parser->symbols, "ASTNode", SYM_TYPEDEF);
		symbol_add(parser->symbols, "FunctionData", SYM_TYPEDEF);
		symbol_add(parser->symbols, "VarDeclData", SYM_TYPEDEF);
		symbol_add(parser->symbols, "TypedefData", SYM_TYPEDEF);
		symbol_add(parser->symbols, "FuncPtrData", SYM_TYPEDEF);
		symbol_add(parser->symbols, "Formatter", SYM_TYPEDEF);
		symbol_add(parser->symbols, "Lexer", SYM_TYPEDEF);
		symbol_add(parser->symbols, "Parser", SYM_TYPEDEF);
		symbol_add(parser->symbols, "Token", SYM_TYPEDEF);
		symbol_add(parser->symbols, "TokenType", SYM_TYPEDEF);
		symbol_add(parser->symbols, "SymbolTable", SYM_TYPEDEF);
		symbol_add(parser->symbols, "Symbol", SYM_TYPEDEF);
		symbol_add(parser->symbols, "NodeType", SYM_TYPEDEF);
	}

	/* Initialize comment buffer */
	parser->pending_comments = NULL;
	parser->pending_comment_count = 0;
	parser->pending_comment_capacity = 0;

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
 * clear_pending_comments - Reset pending comment buffer (used after raw capture)
 * @parser: Parser instance
 */
static void clear_pending_comments(Parser *parser)
{
	if (!parser)
		return;
	parser->pending_comment_count = 0;
}

/*
 * create_unparsed_node - Build a NODE_UNPARSED covering [start_index, end_index)
 * @parser: Parser instance
 * @start_index: Inclusive token index
 * @end_index: Exclusive token index
 *
 * Return: AST node containing verbatim source slice, or NULL on failure
 */
static ASTNode *create_unparsed_node(Parser *parser, int start_index, int end_index)
{
	ASTNode *node;
	RawSegmentData *segment;
	size_t total = 0;
	char *buffer, *cursor;
	int i;
	Token *start_token = NULL;

	if (!parser)
		return (NULL);
	if (start_index < 0)
		start_index = 0;
	if (end_index > parser->token_count)
		end_index = parser->token_count;
	if (start_index >= end_index)
		return (NULL);

	for (i = start_index; i < end_index; i++)
	{
		Token *t = parser->tokens[i];

		if (t && t->lexeme)
			total += strlen(t->lexeme);
	}

	buffer = malloc(total + 1);
	if (!buffer)
		return (NULL);

	cursor = buffer;
	for (i = start_index; i < end_index; i++)
	{
		Token *t = parser->tokens[i];
		size_t len;

		if (!t || !t->lexeme)
			continue;
		len = strlen(t->lexeme);
		memcpy(cursor, t->lexeme, len);
		cursor += len;
	}
	*cursor = '\0';

	segment = malloc(sizeof(RawSegmentData));
	if (!segment)
	{
		free(buffer);
		return (NULL);
	}

	segment->text = buffer;
	segment->start_line = parser->tokens[start_index] ?
		parser->tokens[start_index]->line : 0;
	segment->end_line = parser->tokens[end_index - 1] ?
		parser->tokens[end_index - 1]->line : segment->start_line;

	start_token = parser->tokens[start_index];
	node = ast_node_create(NODE_UNPARSED, start_token);
	if (!node)
	{
		free(buffer);
		free(segment);
		return (NULL);
	}

	node->data = segment;
	return (node);
}

/*
 * recover_top_level - Consume tokens until a safe boundary for raw capture
 * @parser: Parser instance
 * @start_index: Token index to begin capturing from
 *
 * Return: NODE_UNPARSED covering skipped tokens, or NULL on failure
 */
static ASTNode *recover_top_level(Parser *parser, int start_index)
{
	Token *t;
	int brace_depth = 0;

	if (!parser)
		return (NULL);
	if (start_index < 0 || start_index >= parser->token_count)
		start_index = parser->current;

	while (!is_at_end(parser))
	{
		t = peek(parser);
		if (!t)
			break;

		if (t->type == TOK_SEMICOLON && brace_depth == 0)
		{
			advance(parser);
			break;
		}
		if (t->type == TOK_LBRACE)
		{
			brace_depth++;
			advance(parser);
			continue;
		}
		if (t->type == TOK_RBRACE)
		{
			if (brace_depth == 0)
			{
				advance(parser);
				break;
			}
			brace_depth--;
			advance(parser);
			if (brace_depth == 0)
				break;
			continue;
		}

		advance(parser);
	}

	if (parser->current <= start_index && !is_at_end(parser))
		advance(parser);

	return (create_unparsed_node(parser, start_index, parser->current));
}

/*
 * recover_statement - Consume tokens for a statement-level raw capture
 * @parser: Parser instance
 * @start_index: Token index to begin capturing from
 *
 * Return: NODE_UNPARSED covering problematic statement, or NULL
 */
static ASTNode *recover_statement(Parser *parser, int start_index)
{
	Token *t;
	int brace_depth = 0;

	if (!parser)
		return (NULL);
	if (start_index < 0 || start_index >= parser->token_count)
		start_index = parser->current;

	while (!is_at_end(parser))
	{
		t = peek(parser);
		if (!t)
			break;

		if (t->type == TOK_SEMICOLON && brace_depth == 0)
		{
			advance(parser);
			break;
		}
		if (t->type == TOK_NEWLINE && brace_depth == 0)
		{
			advance(parser);
			break;
		}
		if (t->type == TOK_LBRACE)
		{
			brace_depth++;
			advance(parser);
			continue;
		}
		if (t->type == TOK_RBRACE)
		{
			if (brace_depth == 0)
				break;
			brace_depth--;
			advance(parser);
			if (brace_depth == 0)
				break;
			continue;
		}

		advance(parser);
	}

	if (parser->current <= start_index && !is_at_end(parser))
		advance(parser);

	return (create_unparsed_node(parser, start_index, parser->current));
}

/*
 * recover_enum_entry - Capture a problematic enum value safely
 * @parser: Parser instance
 * @start_index: Token index where the enum value began
 */
static ASTNode *recover_enum_entry(Parser *parser, int start_index)
{
	Token *t;
	int brace_depth = 0;
	int paren_depth = 0;

	if (!parser)
		return (NULL);
	if (start_index < 0 || start_index >= parser->token_count)
		start_index = parser->current;

	while (!is_at_end(parser))
	{
		t = peek(parser);
		if (!t)
			break;

		if ((t->type == TOK_COMMA || t->type == TOK_NEWLINE) &&
		    brace_depth == 0 && paren_depth == 0)
		{
			/* Stop before closing brace so caller can consume it */
			if (t->type == TOK_COMMA || t->type == TOK_NEWLINE)
				advance(parser);
			break;
		}
		if (t->type == TOK_RBRACE && brace_depth == 0 && paren_depth == 0)
			break;

		if (t->type == TOK_LBRACE)
			brace_depth++;
		else if (t->type == TOK_RBRACE && brace_depth > 0)
			brace_depth--;
		else if (t->type == TOK_LPAREN)
			paren_depth++;
		else if (t->type == TOK_RPAREN && paren_depth > 0)
			paren_depth--;

		advance(parser);
	}

	if (parser->current <= start_index && !is_at_end(parser))
		advance(parser);

	return (create_unparsed_node(parser, start_index, parser->current));
}

/*
 * add_unparsed_child - Helper to append a recovered raw node to parent
 * @parser: Parser instance
 * @parent: AST parent node
 * @start_index: Token index marking where capture should begin
 */
static void add_unparsed_child(Parser *parser, ASTNode *parent, int start_index)
{
	ASTNode *raw;

	if (!parent)
		return;

	raw = recover_top_level(parser, start_index);
	if (raw)
	{
		raw->blank_lines_before = 0;
		ast_node_add_child(parent, raw);
	}
	clear_pending_comments(parser);
}

/*
 * copy_token_text - Concatenate lexemes for tokens in [start_index, end_index)
 * @parser: Parser instance
 * @start_index: Inclusive start token index
 * @end_index: Exclusive end token index
 *
 * Return: Newly allocated C string or NULL on failure
 */
static char *copy_token_text(Parser *parser, int start_index, int end_index)
{
	size_t total = 0;
	char *buffer, *cursor;
	int i;

	if (!parser)
		return (NULL);
	if (start_index < 0)
		start_index = 0;
	if (end_index > parser->token_count)
		end_index = parser->token_count;
	if (start_index >= end_index)
		return (NULL);

	for (i = start_index; i < end_index; i++)
	{
		Token *tok = parser->tokens[i];

		if (tok && tok->lexeme)
			total += strlen(tok->lexeme);
	}

	buffer = malloc(total + 1);
	if (!buffer)
		return (NULL);

	cursor = buffer;
	for (i = start_index; i < end_index; i++)
	{
		Token *tok = parser->tokens[i];
		size_t len;

		if (!tok || !tok->lexeme)
			continue;
		len = strlen(tok->lexeme);
		memcpy(cursor, tok->lexeme, len);
		cursor += len;
	}
	*cursor = '\0';

	return (buffer);
}

static int token_allowed_in_type(Token *token)
{
	if (!token)
		return (0);

	switch (token->type)
	{
	case TOK_WHITESPACE:
	case TOK_NEWLINE:
	case TOK_COMMENT_LINE:
	case TOK_COMMENT_BLOCK:
	case TOK_IDENTIFIER:
	case TOK_STAR:
	case TOK_CONST:
	case TOK_VOLATILE:
	case TOK_UNSIGNED:
	case TOK_SIGNED:
	case TOK_SHORT:
	case TOK_LONG:
	case TOK_INT:
	case TOK_VOID:
	case TOK_CHAR_KW:
	case TOK_STRUCT:
	case TOK_ENUM:
	case TOK_UNION:
	case TOK_STATIC:
	case TOK_EXTERN:
	case TOK_REGISTER:
	case TOK_AUTO:
	case TOK_LBRACKET:
	case TOK_RBRACKET:
	case TOK_INTEGER:
	case TOK_TYPEDEF:
		return (1);
	default:
		return (0);
	}
}

/*
 * looks_like_type_in_parens - Check if tokens until ) form a type name
 * @parser: Parser instance
 * @start_index: Index of the first token after '('
 * @closing_index: Optional out param receiving index of closing ')'
 */
static int looks_like_type_in_parens(Parser *parser, int start_index,
	int *closing_index)
{
	int i;
	int saw_content = 0;
	int looks_like_type = 1;
	Token *prev_non_ws = NULL;

	if (!parser || start_index < 0 || start_index >= parser->token_count)
		return (0);

	for (i = start_index; i < parser->token_count; i++)
	{
		Token *inner = parser->tokens[i];

		if (!inner)
			break;
		if (inner->type == TOK_RPAREN)
			break;
		if (!token_allowed_in_type(inner))
		{
			looks_like_type = 0;
			break;
		}

		if (inner->type != TOK_WHITESPACE &&
		    inner->type != TOK_NEWLINE &&
		    inner->type != TOK_COMMENT_LINE &&
		    inner->type != TOK_COMMENT_BLOCK)
		{
			saw_content = 1;

			if (inner->type == TOK_IDENTIFIER)
			{
				int ident_is_type = 0;

				if (prev_non_ws &&
				    (prev_non_ws->type == TOK_STRUCT ||
				     prev_non_ws->type == TOK_ENUM ||
				     prev_non_ws->type == TOK_UNION))
				{
					ident_is_type = 1;
				}
				else if (inner->lexeme &&
					 symbol_is_typedef(parser->symbols, inner->lexeme))
				{
					ident_is_type = 1;
				}

				if (!ident_is_type)
				{
					looks_like_type = 0;
					break;
				}
			}

			prev_non_ws = inner;
		}
	}

	if (!looks_like_type || !saw_content ||
	    i >= parser->token_count ||
	    parser->tokens[i]->type != TOK_RPAREN)
		return (0);

	if (closing_index)
		*closing_index = i;

	return (1);
}

/*
 * skip_gnu_attributes - Skip GCC-style __attribute__ annotations
 * @parser: Parser instance
 */
static void skip_gnu_attributes(Parser *parser)
{
	Token *tok;

	if (!parser)
		return;

	while (!is_at_end(parser))
	{
		tok = peek(parser);
		if (!tok || tok->type != TOK_IDENTIFIER || !tok->lexeme ||
		    strcmp(tok->lexeme, "__attribute__") != 0)
			break;

		advance(parser);
		skip_whitespace(parser);

		if (!match(parser, TOK_LPAREN))
			continue;
		advance(parser); /* consume '(' */
		skip_whitespace(parser);

		int depth = 1;
		while (!is_at_end(parser) && depth > 0)
		{
			tok = peek(parser);
			if (!tok)
				break;
			if (tok->type == TOK_LPAREN)
				depth++;
			else if (tok->type == TOK_RPAREN)
				depth--;
			advance(parser);
		}
		skip_whitespace(parser);
	}
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
		int line = token ? token->line :
			(parser->current > 0 && parser->tokens[parser->current - 1] ?
			 parser->tokens[parser->current - 1]->line : 0);

		fprintf(stderr, "Parse error (line %d): expected %s, got %s\n",
			line,
			token_type_to_string(type),
			token ? token_type_to_string(token->type) : "EOF");

		/* Print a short token window to help debug parser state */
		{
			int i, start = parser->current, end = parser->current + 6;
			if (start < 0) start = 0;
			if (end > parser->token_count) end = parser->token_count;
			fprintf(stderr, "  Context tokens (idx: type \"lexeme\"):\n");
			for (i = start; i < end; i++)
			{
				Token *ct = parser->tokens[i];
				if (!ct) continue;
				fprintf(stderr, "    [%d]: %s \"%s\"\n",
						i,
						token_type_to_string(ct->type),
						ct->lexeme ? ct->lexeme : "");
			}
		}
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
 * skip_whitespace - Skip whitespace, newlines, and collect comments
 * @parser: Parser instance
 *
 * Comments are added to parser->pending_comments to be attached to the next node.
 * Returns the number of blank lines encountered.
 */
static int skip_whitespace(Parser *parser)
{
	Token *token;
	int newline_count = 0;

	if (parser)
		parser->whitespace_start = parser->current;

	while (!is_at_end(parser))
	{
		token = peek(parser);
		if (token->type == TOK_WHITESPACE)
			advance(parser);
		else if (token->type == TOK_NEWLINE)
		{
			newline_count++;
			advance(parser);
		}
		else if (token->type == TOK_COMMENT_LINE ||
			 token->type == TOK_COMMENT_BLOCK)
		{
			add_pending_comment(parser, token);
			advance(parser);
		}
		else
			break;
	}

	/* Return number of blank lines (2+ newlines = 1+ blank lines) */
	return (newline_count > 1 ? newline_count - 1 : 0);
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
		int type_start;
		int closing_index = -1;
		char *type_text = NULL;

		advance(parser);
		skip_whitespace(parser);
		type_start = parser->current;

		if (looks_like_type_in_parens(parser, type_start, &closing_index))
		{
			ASTNode *cast_node;
			Token *type_token = parser->tokens[type_start];

			cast_node = ast_node_create(NODE_CAST, type_token);
			if (!cast_node)
				return (NULL);

			if (closing_index > type_start)
				type_text = copy_token_text(parser, type_start, closing_index);
			if (type_text)
				cast_node->data = type_text;

			parser->current = closing_index;
			expect(parser, TOK_RPAREN);
			skip_whitespace(parser);

			node = parse_unary(parser);
			if (node)
				ast_node_add_child(cast_node, node);
			return (cast_node);
		}

		parser->current = type_start;

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
			MemberAccessData *access_data;

			advance(parser); /* consume . or -> */
			skip_whitespace(parser);

			/* Expect an identifier for the member name */
			name_token = expect(parser, TOK_IDENTIFIER);
			if (!name_token)
				return (NULL);

			member = ast_node_create(NODE_MEMBER_ACCESS, name_token);
			if (!member)
				return (NULL);
			access_data = malloc(sizeof(MemberAccessData));
			if (access_data)
			{
				access_data->uses_arrow = (token->type == TOK_ARROW);
				member->data = access_data;
			}
			/* first child is the object, second implicitly the name via token */
			ast_node_add_child(member, node);
			node = member;
			continue;
		}

		/* Postfix ++ or -- */
			if (token->type == TOK_INCREMENT || token->type == TOK_DECREMENT)
			{
				ASTNode *postfix = ast_node_create(NODE_UNARY, token);
				UnaryData *unary_data;

				advance(parser);
				unary_data = malloc(sizeof(UnaryData));
				if (unary_data)
				{
					unary_data->is_postfix = 1;
					postfix->data = unary_data;
				}
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
			int saved_pos;
			int i;
			int looks_like_type = 1;
			char *type_text = NULL;

			advance(parser);
			skip_whitespace(parser);
			saved_pos = parser->current;

			for (i = parser->current; i < parser->token_count; i++)
			{
				Token *inner = parser->tokens[i];

				if (!inner)
					break;
				if (inner->type == TOK_RPAREN)
					break;
				if (!token_allowed_in_type(inner))
				{
					looks_like_type = 0;
					break;
				}
			}

			if (i >= parser->token_count || (parser->tokens[i] && parser->tokens[i]->type != TOK_RPAREN))
				looks_like_type = 0;

			if (looks_like_type)
			{
				type_text = copy_token_text(parser, saved_pos, i);
				if (type_text)
					node->data = type_text;
				parser->current = i;
				expect(parser, TOK_RPAREN);
			}
			else
			{
				parser->current = saved_pos;
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
	int blank_lines;

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
		blank_lines = skip_whitespace(parser);

		if (match(parser, TOK_RBRACE))
			break;

		stmt = parse_statement(parser);
		if (stmt)
		{
			attach_pending_comments(parser, stmt);
			/* Preserve user blank lines (max 1) */
			stmt->blank_lines_before = (blank_lines > 0 ? 1 : 0);
			ast_node_add_child(block, stmt);
		}
		/* Don't skip whitespace here - we do it at the start of the loop */
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
	ASTNode *node = NULL;
	Token **saved_comments = NULL;
	int saved_count = 0;
	int i;
	int statement_start = parser->whitespace_start;
	int start_errors = parser->error_count;

	if (statement_start < 0 || statement_start >= parser->token_count)
		statement_start = parser->current;
	else if (statement_start > parser->current)
		statement_start = parser->current;

	skip_whitespace(parser);
	token = peek(parser);

	if (!token)
		return (NULL);

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
	else if (token->type == TOK_RETURN)
	{
		advance(parser);
		node = ast_node_create(NODE_RETURN, token);
		skip_whitespace(parser);

		if (node && !match(parser, TOK_SEMICOLON))
		{
			ASTNode *expr = parse_expression(parser);

			if (expr)
				ast_node_add_child(node, expr);
			else
			{
				ast_node_destroy(node);
				node = NULL;
			}
		}

		skip_whitespace(parser);
		if (node && !expect(parser, TOK_SEMICOLON))
		{
			ast_node_destroy(node);
			node = NULL;
		}
	}
	else if (token->type == TOK_BREAK)
	{
		node = ast_node_create(NODE_BREAK, token);
		advance(parser);
		skip_whitespace(parser);
		if (node && !expect(parser, TOK_SEMICOLON))
		{
			ast_node_destroy(node);
			node = NULL;
		}
	}
	else if (token->type == TOK_CONTINUE)
	{
		node = ast_node_create(NODE_CONTINUE, token);
		advance(parser);
		skip_whitespace(parser);
		if (node && !expect(parser, TOK_SEMICOLON))
		{
			ast_node_destroy(node);
			node = NULL;
		}
	}
	else if (token->type == TOK_LBRACE)
		node = parse_block(parser);
	else if (token->type == TOK_TYPEDEF)
		node = parse_typedef(parser);
	else if (is_type_keyword(token->type))
		node = parse_var_declaration(parser);
	else if (token->type == TOK_IDENTIFIER &&
		 symbol_is_typedef(parser->symbols, token->lexeme))
		node = parse_var_declaration(parser);
	else if (token->type == TOK_IDENTIFIER && looks_like_ptr_declaration(parser))
		node = parse_var_declaration(parser);
	else
	{
		node = ast_node_create(NODE_EXPR_STMT, NULL);
		if (node)
		{
			ASTNode *expr = parse_expression(parser);

			if (expr)
				ast_node_add_child(node, expr);
			else
			{
				ast_node_destroy(node);
				node = NULL;
			}
		}

		if (node)
		{
			skip_whitespace(parser);
			if (!expect(parser, TOK_SEMICOLON))
			{
				ast_node_destroy(node);
				node = NULL;
			}
		}
	}

	if (!node || parser->error_count > start_errors)
	{
		ASTNode *raw;

		parser->error_count = start_errors;
		if (node)
			ast_node_destroy(node);
		raw = recover_statement(parser, statement_start);

		if (saved_comments)
			free(saved_comments);
		clear_pending_comments(parser);
		return (raw);
	}

	if (saved_comments)
	{
		for (i = 0; i < saved_count; i++)
			ast_node_add_leading_comment(node, saved_comments[i]);
		free(saved_comments);
	}

	collect_trailing_comments(parser, node);
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
	int start_errors = parser->error_count;

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
	if (!node)
		return (NULL);

	/* Parse struct body if present */
	if (match(parser, TOK_LBRACE))
	{
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse struct members as variable declarations */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			int member_start;
			int member_errors;
			ASTNode *raw;

			skip_whitespace(parser);
			if (match(parser, TOK_RBRACE))
				break;

			member_start = parser->current;
			member_errors = parser->error_count;
			/* Parse member as variable declaration */
			member = parse_var_declaration(parser);
			if (member && parser->error_count == member_errors)
				ast_node_add_child(node, member);
			else
			{
				parser->error_count = member_errors;
				if (member)
					ast_node_destroy(member);
				raw = recover_statement(parser, member_start);
				if (raw)
				{
					/* Preserve unhandled members verbatim */
					ast_node_add_child(node, raw);
				}
				else
				{
					ast_node_destroy(node);
					parser->error_count = start_errors;
					return (NULL);
				}
			}
		}

		if (!match(parser, TOK_RBRACE))
		{
			ast_node_destroy(node);
			parser->error_count = start_errors;
			return (NULL);
		}
		advance(parser);
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
	int start_errors = parser->error_count;

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
	if (!node)
		return (NULL);

	/* Parse enum body if present */
	if (match(parser, TOK_LBRACE))
	{
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse enum values */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			int entry_start;
			int entry_errors;
			ASTNode *raw = NULL;

			skip_whitespace(parser);
			entry_start = parser->current;
			entry_errors = parser->error_count;
			if (match(parser, TOK_RBRACE))
				break;

			/* Each enum value is an identifier */
			if (match(parser, TOK_IDENTIFIER))
			{
				Token *ident = advance(parser);

				enum_val = ast_node_create(NODE_ENUM_VALUE, ident);
				if (!enum_val)
				{
					parser->error_count = entry_errors;
					return (NULL);
				}
				skip_whitespace(parser);

				/* Check for = value */
				if (match(parser, TOK_ASSIGN))
				{
					advance(parser);
					skip_whitespace(parser);
					/* Consume the value expression */
					while (!is_at_end(parser) &&
					       !match(parser, TOK_COMMA) &&
					       !match(parser, TOK_RBRACE))
					{
						/* Store value as child if it's a literal */
						if (enum_val->child_count == 0 &&
						    (match(parser, TOK_INTEGER) || match(parser, TOK_IDENTIFIER)))
						{
							ast_node_add_child(enum_val,
								ast_node_create(NODE_LITERAL, peek(parser)));
						}
						advance(parser);
						skip_whitespace(parser);
					}
				}

				if (parser->error_count == entry_errors)
					ast_node_add_child(node, enum_val);
				else
				{
					raw = recover_enum_entry(parser, entry_start);
					parser->error_count = entry_errors;
					ast_node_destroy(enum_val);
					if (raw)
						ast_node_add_child(node, raw);
					else
					{
						ast_node_destroy(node);
						return (NULL);
					}
				}
			}
			else
			{
				raw = recover_enum_entry(parser, entry_start);
				if (raw)
					ast_node_add_child(node, raw);
				else
					break;
			}

			/* Skip comma if present */
			if (match(parser, TOK_COMMA))
			{
				advance(parser);
				skip_whitespace(parser);
			}
		}

		if (!match(parser, TOK_RBRACE))
		{
			ast_node_destroy(node);
			parser->error_count = start_errors;
			return (NULL);
		}
		advance(parser);
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
	ASTNode *member;
	int start_errors = parser->error_count;

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
	if (!node)
		return (NULL);

	/* Parse union body if present */
	if (match(parser, TOK_LBRACE))
	{
		advance(parser); /* consume { */
		skip_whitespace(parser);

		/* Parse union members like struct members */
		while (!is_at_end(parser) && !match(parser, TOK_RBRACE))
		{
			int member_start;
			int member_errors;
			ASTNode *raw;

			skip_whitespace(parser);
			if (match(parser, TOK_RBRACE))
				break;

			member_start = parser->current;
			member_errors = parser->error_count;
			member = parse_var_declaration(parser);
			if (member && parser->error_count == member_errors)
				ast_node_add_child(node, member);
			else
			{
				parser->error_count = member_errors;
				if (member)
					ast_node_destroy(member);
				raw = recover_statement(parser, member_start);
				if (raw)
					ast_node_add_child(node, raw);
				else
				{
					ast_node_destroy(node);
					parser->error_count = start_errors;
					return (NULL);
				}
			}
		}

		if (!match(parser, TOK_RBRACE))
		{
			ast_node_destroy(node);
			parser->error_count = start_errors;
			return (NULL);
		}
		advance(parser);
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

	skip_gnu_attributes(parser);

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
	int start_errors;

	program = ast_node_create(NODE_PROGRAM, NULL);
	if (!program)
		return (NULL);

	while (!is_at_end(parser))
	{
		blank_lines = skip_whitespace(parser);
		int section_start = parser->whitespace_start;

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
				ast_node_add_child(program, pp_node);
			}
			continue;
		}

		/* Try to parse typedef */
		if (match(parser, TOK_TYPEDEF))
		{
			start_errors = parser->error_count;
			func = parse_typedef(parser);
			if (func && parser->error_count == start_errors)
			{
				attach_pending_comments(parser, func);
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				ast_node_add_child(program, func);
			}
			else
			{
				parser->error_count = start_errors;
				if (func)
					ast_node_destroy(func);
				add_unparsed_child(parser, program, section_start);
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
				start_errors = parser->error_count;
				func = parse_struct_definition(parser);
				if (func && parser->error_count == start_errors)
				{
					attach_pending_comments(parser, func);
					func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
					ast_node_add_child(program, func);
					skip_whitespace(parser);
					if (match(parser, TOK_SEMICOLON))
						advance(parser);
				}
				else
				{
					parser->error_count = start_errors;
					if (func)
						ast_node_destroy(func);
					add_unparsed_child(parser, program, section_start);
				}
				continue;
			}
			/* Otherwise fall through to function parsing */
		}

		/* Try to parse enum */
		if (match(parser, TOK_ENUM))
		{
			start_errors = parser->error_count;
			func = parse_enum_definition(parser);
			if (func && parser->error_count == start_errors)
			{
				attach_pending_comments(parser, func);
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				ast_node_add_child(program, func);
				skip_whitespace(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
			else
			{
				parser->error_count = start_errors;
				if (func)
					ast_node_destroy(func);
				add_unparsed_child(parser, program, section_start);
			}
			continue;
		}

		/* Try to parse union */
		if (match(parser, TOK_UNION))
		{
			start_errors = parser->error_count;
			func = parse_union_definition(parser);
			if (func && parser->error_count == start_errors)
			{
				attach_pending_comments(parser, func);
				func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
				ast_node_add_child(program, func);
				skip_whitespace(parser);
				if (match(parser, TOK_SEMICOLON))
					advance(parser);
			}
			else
			{
				parser->error_count = start_errors;
				if (func)
					ast_node_destroy(func);
				add_unparsed_child(parser, program, section_start);
			}
			continue;
		}

		/* Try to parse a function first */
		start_errors = parser->error_count;
		func = parse_function(parser);
		if (func && parser->error_count == start_errors)
		{
			func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
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
				int decl_errors = parser->error_count;
				func = parse_var_declaration(parser);
				if (func && parser->error_count == decl_errors)
				{
					attach_pending_comments(parser, func);
					func->blank_lines_before = (blank_lines > 0 ? 1 : 0);
					ast_node_add_child(program, func);
					continue;
				}
			}

			parser->error_count = start_errors;
			if (func)
				ast_node_destroy(func);
			add_unparsed_child(parser, program, section_start);
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
