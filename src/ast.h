#ifndef AST_H
#define AST_H

#include "token.h"

/*
 * AST node types
 */
typedef enum {
	NODE_PROGRAM,
	NODE_FUNCTION,
	NODE_VAR_DECL,
	NODE_STRUCT,
	NODE_TYPEDEF,
	NODE_ENUM,
	NODE_BLOCK,
	NODE_IF,
	NODE_WHILE,
	NODE_FOR,
	NODE_DO_WHILE,
	NODE_SWITCH,
	NODE_CASE,
	NODE_RETURN,
	NODE_BREAK,
	NODE_CONTINUE,
	NODE_GOTO,
	NODE_LABEL,
	NODE_EXPR_STMT,
	NODE_BINARY,
	NODE_UNARY,
	NODE_CALL,
	NODE_LITERAL,
	NODE_IDENTIFIER,
	NODE_MEMBER_ACCESS,
	NODE_ARRAY_ACCESS,
	NODE_CAST,
	NODE_SIZEOF,
	NODE_TERNARY
} NodeType;

/*
 * AST node structure
 */
typedef struct ASTNode {
	NodeType type;
	Token *token;

	struct ASTNode **children;
	int child_count;
	int child_capacity;

	/* Comments attached to this node */
	Token **leading_comments;
	int leading_comment_count;
	Token **trailing_comments;
	int trailing_comment_count;

	/* Node-specific data */
	void *data;
} ASTNode;

/* AST node creation and destruction */
ASTNode *ast_node_create(NodeType type, Token *token);
void ast_node_destroy(ASTNode *node);

/* Child management */
int ast_node_add_child(ASTNode *parent, ASTNode *child);

/* Comment management */
int ast_node_add_leading_comment(ASTNode *node, Token *comment);
int ast_node_add_trailing_comment(ASTNode *node, Token *comment);

#endif /* AST_H */
