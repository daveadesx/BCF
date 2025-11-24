#include "src/lexer.h"
#include "src/parser.h"
#include "src/ast.h"
#include "src/utils.h"
#include <stdio.h>
#include <stdlib.h>

/* Print AST tree structure */
void print_ast(ASTNode *node, int indent)
{
	int i;

	if (!node)
		return;

	for (i = 0; i < indent; i++)
		printf("  ");

	switch (node->type)
	{
	case NODE_PROGRAM:
		printf("PROGRAM\n");
		break;
	case NODE_FUNCTION:
		printf("FUNCTION: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_VAR_DECL:
		printf("VAR_DECL: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_STRUCT:
		printf("STRUCT: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_TYPEDEF:
		printf("TYPEDEF\n");
		break;
	case NODE_ENUM:
		printf("ENUM: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_BLOCK:
		printf("BLOCK\n");
		break;
	case NODE_IF:
		printf("IF\n");
		break;
	case NODE_WHILE:
		printf("WHILE\n");
		break;
	case NODE_FOR:
		printf("FOR\n");
		break;
	case NODE_DO_WHILE:
		printf("DO_WHILE\n");
		break;
	case NODE_SWITCH:
		printf("SWITCH\n");
		break;
	case NODE_CASE:
		printf("CASE\n");
		break;
	case NODE_RETURN:
		printf("RETURN\n");
		break;
	case NODE_BREAK:
		printf("BREAK\n");
		break;
	case NODE_CONTINUE:
		printf("CONTINUE\n");
		break;
	case NODE_GOTO:
		printf("GOTO\n");
		break;
	case NODE_LABEL:
		printf("LABEL\n");
		break;
	case NODE_EXPR_STMT:
		printf("EXPR_STMT\n");
		break;
	case NODE_BINARY:
		printf("BINARY: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_UNARY:
		printf("UNARY: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_CALL:
		printf("CALL: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_LITERAL:
		printf("LITERAL: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_IDENTIFIER:
		printf("IDENTIFIER: %s\n", node->token ? node->token->lexeme : "?");
		break;
	case NODE_MEMBER_ACCESS:
		printf("MEMBER_ACCESS\n");
		break;
	case NODE_ARRAY_ACCESS:
		printf("ARRAY_ACCESS\n");
		break;
	case NODE_CAST:
		printf("CAST\n");
		break;
	case NODE_SIZEOF:
		printf("SIZEOF\n");
		break;
	case NODE_TERNARY:
		printf("TERNARY\n");
		break;
	default:
		printf("NODE (type %d)\n", node->type);
		break;
	}

	for (i = 0; i < node->child_count; i++)
		print_ast(node->children[i], indent + 1);
}

int main(int argc, char **argv)
{
	char *source;
	Lexer *lexer;
	Parser *parser;
	ASTNode *ast;
	Token **tokens;
	int token_count;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return (1);
	}

	source = read_file(argv[1]);
	if (!source)
	{
		fprintf(stderr, "Error reading file\n");
		return (1);
	}

	printf("Source:\n%s\n\n", source);

	/* Tokenize */
	lexer = lexer_create(source);
	if (!lexer)
	{
		fprintf(stderr, "Error creating lexer\n");
		free(source);
		return (1);
	}

	if (lexer_tokenize(lexer) < 0)
	{
		fprintf(stderr, "Tokenization failed\n");
		lexer_destroy(lexer);
		free(source);
		return (1);
	}

	tokens = lexer_get_tokens(lexer);
	token_count = lexer_get_token_count(lexer);

	printf("Tokenized: %d tokens\n\n", token_count);

	/* Parse */
	parser = parser_create(tokens, token_count);
	if (!parser)
	{
		fprintf(stderr, "Error creating parser\n");
		lexer_destroy(lexer);
		free(source);
		return (1);
	}

	ast = parser_parse(parser);
	if (!ast)
	{
		fprintf(stderr, "Parsing failed\n");
		parser_destroy(parser);
		lexer_destroy(lexer);
		free(source);
		return (1);
	}

	printf("AST Structure:\n");
	print_ast(ast, 0);

	printf("\n✅ Successfully parsed!\n");

	/* Cleanup */
	ast_node_destroy(ast);
	parser_destroy(parser);
	lexer_destroy(lexer);
	free(source);

	return (0);
}
