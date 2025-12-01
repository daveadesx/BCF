#ifndef TOKEN_H
#define TOKEN_H

/*
 * Token types for C lexer
 * Includes all keywords, operators, literals, and special tokens
 */
typedef enum {
	/* Whitespace and formatting */
	TOK_WHITESPACE,
	TOK_NEWLINE,

	/* Comments */
	TOK_COMMENT_LINE,
	TOK_COMMENT_BLOCK,

	/* Preprocessor */
	TOK_PREPROCESSOR,

	/* Identifiers and literals */
	TOK_IDENTIFIER,
	TOK_INTEGER,
	TOK_FLOAT,
	TOK_STRING,
	TOK_CHAR,

	/* Keywords */
	TOK_IF,
	TOK_ELSE,
	TOK_WHILE,
	TOK_FOR,
	TOK_DO,
	TOK_SWITCH,
	TOK_CASE,
	TOK_DEFAULT,
	TOK_BREAK,
	TOK_CONTINUE,
	TOK_RETURN,
	TOK_GOTO,
	TOK_TYPEDEF,
	TOK_STRUCT,
	TOK_UNION,
	TOK_ENUM,
	TOK_SIZEOF,
	TOK_VOID,
	TOK_CHAR_KW,
	TOK_SHORT,
	TOK_INT,
	TOK_LONG,
	TOK_FLOAT_KW,
	TOK_DOUBLE,
	TOK_SIGNED,
	TOK_UNSIGNED,
	TOK_CONST,
	TOK_VOLATILE,
	TOK_STATIC,
	TOK_EXTERN,
	TOK_AUTO,
	TOK_REGISTER,

	/* Operators */
	TOK_PLUS,           /* + */
	TOK_MINUS,          /* - */
	TOK_STAR,           /* * */
	TOK_SLASH,          /* / */
	TOK_PERCENT,        /* % */
	TOK_ASSIGN,         /* = */
	TOK_PLUS_ASSIGN,    /* += */
	TOK_MINUS_ASSIGN,   /* -= */
	TOK_STAR_ASSIGN,    /* *= */
	TOK_SLASH_ASSIGN,   /* /= */
	TOK_PERCENT_ASSIGN, /* %= */
	TOK_AMPERSAND_ASSIGN, /* &= */
	TOK_PIPE_ASSIGN,    /* |= */
	TOK_CARET_ASSIGN,   /* ^= */
	TOK_LSHIFT_ASSIGN,  /* <<= */
	TOK_RSHIFT_ASSIGN,  /* >>= */
	TOK_EQUAL,          /* == */
	TOK_NOT_EQUAL,      /* != */
	TOK_LESS,           /* < */
	TOK_GREATER,        /* > */
	TOK_LESS_EQUAL,     /* <= */
	TOK_GREATER_EQUAL,  /* >= */
	TOK_LOGICAL_AND,    /* && */
	TOK_LOGICAL_OR,     /* || */
	TOK_LOGICAL_NOT,    /* ! */
	TOK_AMPERSAND,      /* & */
	TOK_PIPE,           /* | */
	TOK_CARET,          /* ^ */
	TOK_TILDE,          /* ~ */
	TOK_LSHIFT,         /* << */
	TOK_RSHIFT,         /* >> */
	TOK_INCREMENT,      /* ++ */
	TOK_DECREMENT,      /* -- */
	TOK_ARROW,          /* -> */
	TOK_DOT,            /* . */
	TOK_ELLIPSIS,       /* ... */
	TOK_QUESTION,       /* ? */
	TOK_COLON,          /* : */

	/* Punctuation */
	TOK_LPAREN,         /* ( */
	TOK_RPAREN,         /* ) */
	TOK_LBRACE,         /* { */
	TOK_RBRACE,         /* } */
	TOK_LBRACKET,       /* [ */
	TOK_RBRACKET,       /* ] */
	TOK_SEMICOLON,      /* ; */
	TOK_COMMA,          /* , */

	/* Special */
	TOK_EOF,
	TOK_ERROR
} TokenType;

/*
 * Token structure
 * Contains all information about a lexed token
 */
typedef struct {
	TokenType type;
	char *lexeme;
	int line;
	int column;
	int length;
} Token;

/* Token creation and destruction */
Token *token_create(TokenType type, const char *lexeme, int line, int column);
void token_destroy(Token *token);
const char *token_type_to_string(TokenType type);

#endif /* TOKEN_H */
