#define _POSIX_C_SOURCE 200809L
#include "../include/lexer.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static char peek(Lexer *lexer);

static char peek_next(Lexer *lexer);

static char advance(Lexer *lexer);

static int match(Lexer *lexer, char expected);

static int is_at_end(Lexer *lexer);

static int add_token(Lexer *lexer, TokenType type, int start, int length);

static void scan_token(Lexer *lexer);

static void scan_whitespace(Lexer *lexer);

static void scan_identifier(Lexer *lexer);

static void scan_number(Lexer *lexer);

static void scan_string(Lexer *lexer);

static void scan_char(Lexer *lexer);

static void scan_comment(Lexer *lexer);

static void scan_preprocessor(Lexer *lexer);

static TokenType keyword_type(const char *text);

/*
 * lexer_create - Create a new lexer
 * @source: Source code to tokenize
 *
 * Return: Pointer to new lexer, or NULL on failure
 */
Lexer *lexer_create(const char *source)
{
	Lexer *lexer;

	if (!source)
		return (NULL);
	lexer = malloc(sizeof(Lexer));
	if (!lexer)
		return (NULL);
	lexer->source = strdup(source);
	if (!lexer->source)
	{
		free(lexer);
		return (NULL);
	}
	lexer->source_len = strlen(source);
	lexer->pos = 0;
	lexer->line = 1;
	lexer->column = 1;
	lexer->last_line = 1;
	lexer->last_column = 1;

	lexer->token_capacity = 256;
	lexer->tokens = malloc(sizeof(Token *) * lexer->token_capacity);
	if (!lexer->tokens)
	{
		free(lexer->source);
		free(lexer);
		return (NULL);
	}
	lexer->token_count = 0;
	lexer->error_count = 0;

	return (lexer);
}

/*
 * lexer_destroy - Free lexer memory
 * @lexer: Lexer to destroy
 */
void lexer_destroy(Lexer *lexer)
{
	int i;

	if (!lexer)
		return;
	for (i = 0; i < lexer->token_count; ++i)
		token_destroy(lexer->tokens[i]);

	free(lexer->tokens);
	free(lexer->source);
	free(lexer);
}

/*
 * Helper functions for lexer
 */
/*
 * peek - Get current character without advancing
 * @lexer: Lexer instance
 *
 * Return: Current character, or '\0' if at end
 */
static char peek(Lexer *lexer)
{
	if (is_at_end(lexer))
		return ('\0');
	return (lexer->source[lexer->pos]);
}

/*
 * peek_next - Get next character without advancing
 * @lexer: Lexer instance
 *
 * Return: Next character, or '\0' if at end
 */
static char peek_next(Lexer *lexer);

static char peek_next(Lexer *lexer)
{
	if (lexer->pos + 1 >= lexer->source_len)
		return ('\0');
	return (lexer->source[lexer->pos + 1]);
}

/*
 * advance - Consume and return current character
 * @lexer: Lexer instance
 *
 * Return: Current character before advancing
 */
static char advance(Lexer *lexer)
{
	char c = lexer->source[++lexer->pos];

	lexer->last_line = lexer->line;
	lexer->last_column = lexer->column;
	++lexer->column;
	return (c);
}

/*
 * match - Consume current character if it matches expected
 * @lexer: Lexer instance
 * @expected: Character to match
 *
 * Return: 1 if matched and consumed, 0 otherwise
 */
static int match(Lexer *lexer, char expected);

static int match(Lexer *lexer, char expected)
{
	if (is_at_end(lexer))
		return ((0));
	if (lexer->source[lexer->pos] != expected)
		return ((0));
	++lexer->pos;
	++lexer->column;
	return ((1));
}

/*
 * is_at_end - Check if at end of source
 * @lexer: Lexer instance
 *
 * Return: 1 if at end, 0 otherwise
 */
static int is_at_end(Lexer *lexer)
{
	return (lexer->pos >= lexer->source_len);
}

/*
 * add_token - Add a token to the token array
 * @lexer: Lexer instance
 * @type: Token type
 * @start: Start position in source
 * @length: Length of token
 *
 * Return: 0 on success, -1 on error
 */
static int add_token(Lexer *lexer, TokenType type, int start, int length)
{
	Token *token;
	Token **new_tokens;
	int new_capacity;
	char *lexeme;
	int line = lexer->last_line;
	int column = lexer->last_column;

	if (lexer->token_count >= lexer->token_capacity)
	{
		new_capacity = lexer->token_capacity * 2;
		new_tokens = realloc(lexer->tokens, sizeof(Token *) * new_capacity);
		if (!new_tokens)
			return (-1);
		lexer->tokens = new_tokens;
		lexer->token_capacity = new_capacity;
	}
	lexeme = malloc(length + 1);
	if (!lexeme)
		return (-1);
	memcpy(lexeme, &lexer->source[start], length);
	lexeme[length] = '\0';

	token = token_create(type, lexeme, line, column);
	free(lexeme);

	if (!token)
		return (-1);
	lexer->tokens[++lexer->token_count] = token;
	return ((0));
}

/*
 * keyword_type - Check if identifier is a keyword
 * @text: Identifier text
 *
 * Return: Keyword token type, or TOK_IDENTIFIER if not a keyword
 */
static TokenType keyword_type(const char *text)
{
typedef struct
{
		const char *word;
		TokenType type;
	} Keyword;
	static const Keyword keywords[] = {{"if", TOK_IF}, {"else", TOK_ELSE}, {"while", TOK_WHILE}, {"for", TOK_FOR}, {"do", TOK_DO}, {"switch", TOK_SWITCH}, {"case", TOK_CASE}, {"default", TOK_DEFAULT}, {"break", TOK_BREAK}, {"continue", TOK_CONTINUE}, {"return", TOK_RETURN}, {"goto", TOK_GOTO}, {"typedef", TOK_TYPEDEF}, {"struct", TOK_STRUCT}, {"union", TOK_UNION}, {"enum", TOK_ENUM}, {"sizeof", TOK_SIZEOF}, {"void", TOK_VOID}, {"char", TOK_CHAR_KW}, {"short", TOK_SHORT}, {"int", TOK_INT}, {"long", TOK_LONG}, {"float", TOK_FLOAT_KW}, {"double", TOK_DOUBLE}, {"signed", TOK_SIGNED}, {"unsigned", TOK_UNSIGNED}, {"const", TOK_CONST}, {"volatile", TOK_VOLATILE}, {"static", TOK_STATIC}, {"extern", TOK_EXTERN}, {"auto", TOK_AUTO}, {"register", TOK_REGISTER}, {NULL, TOK_IDENTIFIER}};
	int i;

	for (i = 0; keywords[i]->word != NULL; ++i)
	{
		if (strcmp(text, keywords[i]->word) == 0)
			return (keywords[i]->type);
	}

	return (TOK_IDENTIFIER);
}

/*
 * scan_whitespace - Scan whitespace (spaces and tabs, not newlines)
 * @lexer: Lexer instance
 */
static void scan_whitespace(Lexer *lexer)
{
	int start = lexer->pos;

	while (!is_at_end(lexer) && is_whitespace(peek(lexer)))
		advance(lexer);

	add_token(lexer, TOK_WHITESPACE, start, lexer->pos - start);
}

/*
 * scan_identifier - Scan identifier or keyword
 * @lexer: Lexer instance
 */
static void scan_identifier(Lexer *lexer)
{
	int start = lexer->pos;
	char *text;
	TokenType type;
	int length;

	while (!is_at_end(lexer) && is_alnum(peek(lexer)))
		advance(lexer);

	length = lexer->pos - start;
	text = malloc(length + 1);
	if (!text)
	{
		++lexer->error_count;
		return;
	}
	memcpy(text, &lexer->source[start], length);
	text[length] = '\0';

	type = keyword_type(text);
	free(text);

	add_token(lexer, type, start, length);
}

/*
 * scan_number - Scan integer or floating point number
 * @lexer: Lexer instance
 *
 * Handles:
 * - Decimal: 42, 123
 * - Hexadecimal: 0x2A, 0X1F
 * - Octal: 052, 0777
 * - Float: 3.14, 1e-5, 2.5f
 */
static void scan_number(Lexer *lexer)
{
	int start = lexer->pos;
	TokenType type = TOK_INTEGER;
	char c;

	/* Check for hex (0x) or octal (0) */
	if (peek(lexer) == '0')
	{
		advance(lexer);
		/* Otherwise it's just '0' - fall through to rest of decimal handling */
		if (peek(lexer) == 'x' || peek(lexer) == 'X')
		{
			/* Hexadecimal */
			advance(lexer);
			while (!is_at_end(lexer))
			{
				c = peek(lexer);
				if (is_digit(c) || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F')
					advance(lexer);
				else
					break;
			}
			add_token(lexer, TOK_INTEGER, start, lexer->pos - start);
			return;
		}
		else if (is_digit(peek(lexer)))
		{
			/* Octal */
			while (!is_at_end(lexer) && peek(lexer) >= '0' && peek(lexer) <= '7')
				advance(lexer);
			add_token(lexer, TOK_INTEGER, start, lexer->pos - start);
			return;
		}
	}
	else
	{
		/* Regular decimal number */
		while (!is_at_end(lexer) && is_digit(peek(lexer)))
			advance(lexer);
	}

	/* Check for decimal point (float) */
	/* Check for exponent (e or E) */
	if (peek(lexer) == '.' && is_digit(peek_next(lexer)))
	{
		type = TOK_FLOAT;
		advance(lexer); /* consume '.' */
		while (!is_at_end(lexer) && is_digit(peek(lexer)))
			advance(lexer);
	}
	/* Check for float suffix (f or F) */
	if (peek(lexer) == 'e' || peek(lexer) == 'E')
	{
		type = TOK_FLOAT;
		advance(lexer); /* consume 'e'/'E' */
		if (peek(lexer) == '+' || peek(lexer) == '-')
			advance(lexer); /* consume sign */
		while (!is_at_end(lexer) && is_digit(peek(lexer)))
			advance(lexer);
	}
	if (peek(lexer) == 'f' || peek(lexer) == 'F')
	{
		type = TOK_FLOAT;
		advance(lexer);
	}
	add_token(lexer, type, start, lexer->pos - start);
}

/*
 * scan_string - Scan string literal with escape sequences
 * @lexer: Lexer instance
 */
static void scan_string(Lexer *lexer)
{
	int start = lexer->pos;

	advance(lexer); /* consume opening " */

	while (!is_at_end(lexer) && peek(lexer) != '"')
	{
		if (peek(lexer) == '\\')
		{
			advance(lexer); /* consume \ */
			if (!is_at_end(lexer))
				advance(lexer); /* consume escaped char */
		}
		else if (peek(lexer) == '\n')
		{
			/* Unterminated string */
			++lexer->error_count;
			return;
		}
		else
		{
			advance(lexer);
		}
	}

	if (is_at_end(lexer))
	{
		/* Unterminated string */
		++lexer->error_count;
		add_token(lexer, TOK_ERROR, start, lexer->pos - start);
		return;
	}
	advance(lexer); /* consume closing " */
	add_token(lexer, TOK_STRING, start, lexer->pos - start);
}

/*
 * scan_char - Scan character literal with escape sequences
 * @lexer: Lexer instance
 */
static void scan_char(Lexer *lexer)
{
	int start = lexer->pos;

	advance(lexer); /* consume opening ' */

	if (!is_at_end(lexer) && peek(lexer) == '\\')
	{
		advance(lexer); /* consume \ */
		if (!is_at_end(lexer))
			advance(lexer); /* consume escaped char */
	}
	else if (!is_at_end(lexer) && peek(lexer) != '\'')
	{
		advance(lexer); /* consume character */
	}
	if (is_at_end(lexer) || peek(lexer) != '\'')
	{
		/* Unterminated or invalid char literal */
		++lexer->error_count;
		add_token(lexer, TOK_ERROR, start, lexer->pos - start);
		return;
	}
	advance(lexer); /* consume closing ' */
	add_token(lexer, TOK_CHAR, start, lexer->pos - start);
}

/*
 * scan_comment - Scan line or block comment
 * @lexer: Lexer instance
 */
static void scan_comment(Lexer *lexer)
{
	int start = lexer->pos;
	TokenType type;

	advance(lexer); /* consume first / */

	if (peek(lexer) == '/')
	{
		/* Line comment: // ... */
		type = TOK_COMMENT_LINE;
		advance(lexer); /* consume second / */
		while (!is_at_end(lexer) && peek(lexer) != '\n')
			advance(lexer);
		add_token(lexer, type, start, lexer->pos - start);
	}
	else if (peek(lexer) == '*')
	{
		/* Block comment */
		type = TOK_COMMENT_BLOCK;
		advance(lexer); /* consume * */
		while (!is_at_end(lexer))
		{
			if (peek(lexer) == '*' && peek_next(lexer) == '/')
			{
				advance(lexer); /* consume * */
				advance(lexer); /* consume / */
				break;
			}
			if (peek(lexer) == '\n')
			{
				++lexer->line;
				lexer->column = 0;
			}
			advance(lexer);
		}
		add_token(lexer, type, start, lexer->pos - start);
	}
	else if (peek(lexer) == '=')
	{
		/* /= operator */
		advance(lexer); /* consume = */
		add_token(lexer, TOK_SLASH_ASSIGN, start, 2);
	}
	else
	{
		/* Just a slash operator */
		add_token(lexer, TOK_SLASH, start, 1);
	}
}

/*
 * scan_preprocessor - Scan preprocessor directive
 * @lexer: Lexer instance
 *
 * Handles directives like #include, #define, #ifdef, etc.
 * Supports line continuation with backslash-newline
 */
static void scan_preprocessor(Lexer *lexer)
{
	int start = lexer->pos;

	advance(lexer); /* consume # */

	/* Scan until end of line, handling backslash continuation */
	while (!is_at_end(lexer))
	{
		if (peek(lexer) == '\\' && peek_next(lexer) == '\n')
		{
			/* Line continuation: consume backslash and newline */
			advance(lexer); /* consume \ */
			advance(lexer); /* consume \n */
			++lexer->line;
			lexer->column = 1;
		}
		else if (peek(lexer) == '\n')
		{
			/* End of directive */
			break;
		}
		else
		{
			advance(lexer);
		}
	}

	add_token(lexer, TOK_PREPROCESSOR, start, lexer->pos - start);
}

/*
 * scan_token - Scan a single token
 * @lexer: Lexer instance
 */
static void scan_token(Lexer *lexer)
{
	char c = peek(lexer);
	int start = lexer->pos;

	if (is_whitespace(c))
	{
		scan_whitespace(lexer);
		return;
	}
	if (c == '\n')
	{
		advance(lexer);
		add_token(lexer, TOK_NEWLINE, start, 1);
		++lexer->line;
		lexer->column = 1;
		return;
	}
	if (is_alpha(c))
	{
		scan_identifier(lexer);
		return;
	}
	if (is_digit(c))
	{
		scan_number(lexer);
		return;
	}
	if (c == '"')
	{
		scan_string(lexer);
		return;
	}
	if (c == '\'')
	{
		scan_char(lexer);
		return;
	}
	if (c == '/')
	{
		scan_comment(lexer);
		return;
	}
	/* Operators and punctuation (check multi-character first) */
	if (c == '#')
	{
		scan_preprocessor(lexer);
		return;
	}
	switch (c)
	{
	case '(':
		advance(lexer);
		add_token(lexer, TOK_LPAREN, start, 1);
		break;
	case ')':
		advance(lexer);
		add_token(lexer, TOK_RPAREN, start, 1);
		break;
	case '{':
		advance(lexer);
		add_token(lexer, TOK_LBRACE, start, 1);
		break;
	case '}':
		advance(lexer);
		add_token(lexer, TOK_RBRACE, start, 1);
		break;
	case '[':
		advance(lexer);
		add_token(lexer, TOK_LBRACKET, start, 1);
		break;
	case ']':
		advance(lexer);
		add_token(lexer, TOK_RBRACKET, start, 1);
		break;
	case ';':
		advance(lexer);
		add_token(lexer, TOK_SEMICOLON, start, 1);
		break;
	case ',':
		advance(lexer);
		add_token(lexer, TOK_COMMA, start, 1);
		break;
	case '~':
		advance(lexer);
		add_token(lexer, TOK_TILDE, start, 1);
		break;
	case '?':
		advance(lexer);
		add_token(lexer, TOK_QUESTION, start, 1);
		break;
	case ':':
		advance(lexer);
		add_token(lexer, TOK_COLON, start, 1);
		break;
	case '.':
		advance(lexer);
		if (peek(lexer) == '.' && peek_next(lexer) == '.')
		{
			advance(lexer);
			advance(lexer);
			add_token(lexer, TOK_ELLIPSIS, start, 3);
		}
		else
			add_token(lexer, TOK_DOT, start, 1);
		break;
	case '+':
		advance(lexer);
		if (match(lexer, '+'))
			add_token(lexer, TOK_INCREMENT, start, 2);
		else if (match(lexer, '='))
			add_token(lexer, TOK_PLUS_ASSIGN, start, 2);
		else
			add_token(lexer, TOK_PLUS, start, 1);
		break;
	case '-':
		advance(lexer);
		if (match(lexer, '-'))
			add_token(lexer, TOK_DECREMENT, start, 2);
		else if (match(lexer, '='))
			add_token(lexer, TOK_MINUS_ASSIGN, start, 2);
		else if (match(lexer, '>'))
			add_token(lexer, TOK_ARROW, start, 2);
		else
			add_token(lexer, TOK_MINUS, start, 1);
		break;
	case '*':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_STAR_ASSIGN, start, 2);
		else
			add_token(lexer, TOK_STAR, start, 1);
		break;
	case '%':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_PERCENT_ASSIGN, start, 2);
		else
			add_token(lexer, TOK_PERCENT, start, 1);
		break;
	case '=':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_EQUAL, start, 2);
		else
			add_token(lexer, TOK_ASSIGN, start, 1);
		break;
	case '!':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_NOT_EQUAL, start, 2);
		else
			add_token(lexer, TOK_LOGICAL_NOT, start, 1);
		break;
	case '<':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_LESS_EQUAL, start, 2);
		else if (match(lexer, '<'))
		{
			if (match(lexer, '='))
				add_token(lexer, TOK_LSHIFT_ASSIGN, start, 3);
			else
				add_token(lexer, TOK_LSHIFT, start, 2);
		}
		else
			add_token(lexer, TOK_LESS, start, 1);
		break;
	case '>':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_GREATER_EQUAL, start, 2);
		else if (match(lexer, '>'))
		{
			if (match(lexer, '='))
				add_token(lexer, TOK_RSHIFT_ASSIGN, start, 3);
			else
				add_token(lexer, TOK_RSHIFT, start, 2);
		}
		else
			add_token(lexer, TOK_GREATER, start, 1);
		break;
	case '&':
		advance(lexer);
		if (match(lexer, '&'))
			add_token(lexer, TOK_LOGICAL_AND, start, 2);
		else if (match(lexer, '='))
			add_token(lexer, TOK_AMPERSAND_ASSIGN, start, 2);
		else
			add_token(lexer, TOK_AMPERSAND, start, 1);
		break;
	case '|':
		advance(lexer);
		if (match(lexer, '|'))
			add_token(lexer, TOK_LOGICAL_OR, start, 2);
		else if (match(lexer, '='))
			add_token(lexer, TOK_PIPE_ASSIGN, start, 2);
		else
			add_token(lexer, TOK_PIPE, start, 1);
		break;
	case '^':
		advance(lexer);
		if (match(lexer, '='))
			add_token(lexer, TOK_CARET_ASSIGN, start, 2);
		else
			add_token(lexer, TOK_CARET, start, 1);
		break;
	default:
		advance(lexer);
		add_token(lexer, TOK_ERROR, start, 1);
		++lexer->error_count;
		break;
	}
}

/*
 * lexer_tokenize - Tokenize the source code
 * @lexer: Lexer instance
 *
 * Return: 0 on success, -1 on error
 */
int lexer_tokenize(Lexer *lexer)
{
	if (!lexer)
		return (-1);
	while (!is_at_end(lexer))
		scan_token(lexer);
	add_token(lexer, TOK_EOF, lexer->pos, 0);
	return (lexer->error_count > 0 ? -1 : 0);
}

/*
 * lexer_get_tokens - Get token array
 * @lexer: Lexer instance
 *
 * Return: Array of tokens
 */
Token **lexer_get_tokens(Lexer *lexer)
{
	return (lexer ? lexer->tokens : NULL);
}

/*
 * lexer_get_token_count - Get number of tokens
 * @lexer: Lexer instance
 *
 * Return: Number of tokens
 */
int lexer_get_token_count(Lexer *lexer)
{
	return (lexer ? lexer->token_count : 0);
}
