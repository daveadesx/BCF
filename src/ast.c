#include "../include/ast.h"
#include <stdlib.h>

#define INITIAL_CHILD_CAPACITY 8

/*
 * ast_node_create - Create a new AST node
 * @type: Node type
 * @token: Associated token (can be NULL)
 *
 * Return: Pointer to new node, or NULL on failure
 */
ASTNode *ast_node_create(NodeType type, Token *token)
{
	ASTNode *node;

	node = malloc(sizeof(ASTNode));
	if (!node)
		return (NULL);

	node->type = type;
	node->token = token;

	node->child_capacity = INITIAL_CHILD_CAPACITY;
	node->children = malloc(sizeof(ASTNode *) * node->child_capacity);
	if (!node->children)
	{
		free(node);
		return (NULL);
	}

	node->child_count = 0;
	node->leading_comments = NULL;
	node->leading_comment_count = 0;
	node->trailing_comments = NULL;
	node->trailing_comment_count = 0;
	node->data = NULL;

	return (node);
}

/*
 * ast_node_destroy - Free AST node and all children
 * @node: Node to destroy
 */
void ast_node_destroy(ASTNode *node)
{
	int i;

	if (!node)
		return;

	for (i = 0; i < node->child_count; i++)
		ast_node_destroy(node->children[i]);

	free(node->children);
	free(node->leading_comments);
	free(node->trailing_comments);
	/* Note: data field is not freed as it typically points to tokens owned by lexer */
	free(node);
}

/*
 * ast_node_add_child - Add a child node
 * @parent: Parent node
 * @child: Child node to add
 *
 * Return: 0 on success, -1 on failure
 */
int ast_node_add_child(ASTNode *parent, ASTNode *child)
{
	ASTNode **new_children;
	int new_capacity;

	if (!parent || !child)
		return (-1);

	if (parent->child_count >= parent->child_capacity)
	{
		new_capacity = parent->child_capacity * 2;
		new_children = realloc(parent->children,
				       sizeof(ASTNode *) * new_capacity);
		if (!new_children)
			return (-1);

		parent->children = new_children;
		parent->child_capacity = new_capacity;
	}

	parent->children[parent->child_count++] = child;
	return (0);
}

/*
 * ast_node_add_leading_comment - Add leading comment to node
 * @node: AST node
 * @comment: Comment token
 *
 * Return: 0 on success, -1 on failure
 */
int ast_node_add_leading_comment(ASTNode *node, Token *comment)
{
	Token **new_comments;
	int new_count;

	if (!node || !comment)
		return (-1);

	new_count = node->leading_comment_count + 1;
	new_comments = realloc(node->leading_comments,
			       sizeof(Token *) * new_count);
	if (!new_comments)
		return (-1);

	new_comments[node->leading_comment_count] = comment;
	node->leading_comments = new_comments;
	node->leading_comment_count = new_count;

	return (0);
}

/*
 * ast_node_add_trailing_comment - Add trailing comment to node
 * @node: AST node
 * @comment: Comment token
 *
 * Return: 0 on success, -1 on failure
 */
int ast_node_add_trailing_comment(ASTNode *node, Token *comment)
{
	Token **new_comments;
	int new_count;

	if (!node || !comment)
		return (-1);

	new_count = node->trailing_comment_count + 1;
	new_comments = realloc(node->trailing_comments,
			       sizeof(Token *) * new_count);
	if (!new_comments)
		return (-1);

	new_comments[node->trailing_comment_count] = comment;
	node->trailing_comments = new_comments;
	node->trailing_comment_count = new_count;

	return (0);
}
