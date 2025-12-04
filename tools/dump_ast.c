/*
 * dump_ast.c - Debug tool to print parser AST output
 */
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/token.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * node_type_to_string - Convert NodeType to string
 */
static const char *node_type_to_string(NodeType type)
{
	switch (type)
	{
	case NODE_PROGRAM: return "PROGRAM";
	case NODE_FUNCTION: return "FUNCTION";
	case NODE_VAR_DECL: return "VAR_DECL";
	case NODE_STRUCT: return "STRUCT";
	case NODE_TYPEDEF: return "TYPEDEF";
	case NODE_ENUM: return "ENUM";
	case NODE_BLOCK: return "BLOCK";
	case NODE_IF: return "IF";
	case NODE_WHILE: return "WHILE";
	case NODE_FOR: return "FOR";
	case NODE_DO_WHILE: return "DO_WHILE";
	case NODE_SWITCH: return "SWITCH";
	case NODE_CASE: return "CASE";
	case NODE_RETURN: return "RETURN";
	case NODE_BREAK: return "BREAK";
	case NODE_CONTINUE: return "CONTINUE";
	case NODE_GOTO: return "GOTO";
	case NODE_LABEL: return "LABEL";
	case NODE_EXPR_STMT: return "EXPR_STMT";
	case NODE_BINARY: return "BINARY";
	case NODE_UNARY: return "UNARY";
	case NODE_CALL: return "CALL";
	case NODE_LITERAL: return "LITERAL";
	case NODE_IDENTIFIER: return "IDENTIFIER";
	case NODE_MEMBER_ACCESS: return "MEMBER_ACCESS";
	case NODE_ARRAY_ACCESS: return "ARRAY_ACCESS";
	case NODE_CAST: return "CAST";
	case NODE_SIZEOF: return "SIZEOF";
	case NODE_TERNARY: return "TERNARY";
	case NODE_FUNC_PTR: return "FUNC_PTR";
	case NODE_PREPROCESSOR: return "PREPROCESSOR";
	case NODE_TYPE_EXPR: return "TYPE_EXPR";
	case NODE_INIT_LIST: return "INIT_LIST";
	case NODE_UNPARSED: return "UNPARSED";
	default: return "UNKNOWN";
	}
}

/*
 * print_ast - Recursively print AST
 */
static void print_ast(ASTNode *node, int depth)
{
	int i;

	if (!node)
		return;

	/* Print indentation */
	for (i = 0; i < depth; i++)
		printf("  ");

	/* Print node type */
	printf("%s", node_type_to_string(node->type));

	/* Print token info if present */
	if (node->token && node->token->lexeme)
		printf(" \"%s\"", node->token->lexeme);

	/* Print child count */
	if (node->child_count > 0)
		printf(" [%d children]", node->child_count);

	printf("\n");

	/* Recursively print children */
	for (i = 0; i < node->child_count; i++)
		print_ast(node->children[i], depth + 1);
}

/*
 * main - Parse a file and print AST
 */
int main(int argc, char **argv)
{
	char *source;
	Lexer *lexer;
	Parser *parser;
	ASTNode *ast;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <file.c>\n", argv[0]);
		return (1);
	}

	source = read_file(argv[1]);
	if (!source)
	{
		fprintf(stderr, "Error: Could not read file '%s'\n", argv[1]);
		return (1);
	}

	lexer = lexer_create(source);
	if (!lexer)
	{
		fprintf(stderr, "Error: Failed to create lexer\n");
		free(source);
		return (1);
	}

	if (lexer_tokenize(lexer) < 0)
	{
		fprintf(stderr, "Error: Tokenization failed\n");
		lexer_destroy(lexer);
		free(source);
		return (1);
	}

	parser = parser_create(lexer_get_tokens(lexer),
			       lexer_get_token_count(lexer));
	if (!parser)
	{
		fprintf(stderr, "Error: Failed to create parser\n");
		lexer_destroy(lexer);
		free(source);
		return (1);
	}

	printf("=== AST for %s ===\n\n", argv[1]);

	ast = parser_parse(parser);
	if (ast)
	{
		print_ast(ast, 0);
		ast_node_destroy(ast);
	}
	else
	{
		printf("Failed to parse (errors: %d)\n", parser->error_count);
	}

	parser_destroy(parser);
	lexer_destroy(lexer);
	free(source);

	return (0);
}
