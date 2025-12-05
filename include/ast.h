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
	NODE_ENUM_VALUE,
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
	NODE_TERNARY,
	NODE_PARAM,
	NODE_INIT_LIST,
	NODE_FUNC_PTR,
	NODE_TYPE_EXPR,  /* Type used as expression (e.g., in va_arg) */
	NODE_PREPROCESSOR,  /* Preprocessor directive (#include, #define, etc.) */
	NODE_UNPARSED      /* Raw source preserved when parsing fails */
} NodeType;

/* Raw segment data for unparsed source regions */
typedef struct RawSegmentData {
	char *text;
	int start_line;
	int end_line;
} RawSegmentData;

/*
 * Function signature data
 */
typedef struct FunctionData {
	Token **return_type_tokens;  /* Array of tokens forming return type */
	int return_type_count;
	struct ASTNode **params;     /* Array of parameter nodes */
	int param_count;
} FunctionData;

/*
 * Variable declaration data
 */
typedef struct VarDeclData {
	Token **type_tokens;     /* Array of tokens forming type (int, *, const, etc.) */
	int type_count;
	Token *name_token;       /* Variable name */
	Token **array_tokens;    /* Array brackets and size */
	int array_count;
	struct VarDeclData **extra_vars;  /* Additional vars in comma list */
	int extra_count;
	struct ASTNode *init_expr;  /* Initialization expression */
} VarDeclData;

/*
 * Typedef data (for simple typedefs like "typedef int myint;")
 */
typedef struct TypedefData {
	Token **base_type_tokens;  /* Array of tokens forming base type */
	int base_type_count;
} TypedefData;

/*
 * Function pointer data
 */
typedef struct FuncPtrData {
	Token **return_type_tokens;  /* Return type tokens */
	int return_type_count;
	Token *name_token;           /* Function pointer name */
	Token **param_tokens;        /* All tokens in parameter list */
	int param_count;
} FuncPtrData;

typedef struct MemberAccessData {
	int uses_arrow;  /* 1 if operator was ->, 0 if . */
} MemberAccessData;

typedef struct UnaryData {
	int is_postfix;  /* 1 for postfix ++/--, 0 for prefix */
} UnaryData;

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

	/* Blank lines before this node (user-added, max 1 preserved) */
	int blank_lines_before;

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
