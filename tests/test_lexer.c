#include "../src/lexer.h"
#include "../src/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Color codes for output */
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

/*
 * assert_token - Check if token matches expected type and lexeme
 */
static int assert_token(Token *token, TokenType expected_type,
			const char *expected_lexeme, const char *test_name)
{
	tests_run++;

	if (!token)
	{
		printf(RED "✗ FAIL" RESET " %s: token is NULL\n", test_name);
		tests_failed++;
		return (0);
	}

	if (token->type != expected_type)
	{
		printf(RED "✗ FAIL" RESET " %s: expected type %s, got %s\n",
		       test_name,
		       token_type_to_string(expected_type),
		       token_type_to_string(token->type));
		tests_failed++;
		return (0);
	}

	if (expected_lexeme && strcmp(token->lexeme, expected_lexeme) != 0)
	{
		printf(RED "✗ FAIL" RESET " %s: expected lexeme '%s', got '%s'\n",
		       test_name, expected_lexeme, token->lexeme);
		tests_failed++;
		return (0);
	}

	tests_passed++;
	return (1);
}

/*
 * test_keywords - Test all C keywords
 */
static void test_keywords(void)
{
	const char *source = "int char void if else while for return";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Keywords" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_INT, "int", "keyword int");
	assert_token(tokens[2], TOK_CHAR_KW, "char", "keyword char");
	assert_token(tokens[4], TOK_VOID, "void", "keyword void");
	assert_token(tokens[6], TOK_IF, "if", "keyword if");
	assert_token(tokens[8], TOK_ELSE, "else", "keyword else");
	assert_token(tokens[10], TOK_WHILE, "while", "keyword while");
	assert_token(tokens[12], TOK_FOR, "for", "keyword for");
	assert_token(tokens[14], TOK_RETURN, "return", "keyword return");

	lexer_destroy(lexer);
}

/*
 * test_identifiers - Test identifier recognition
 */
static void test_identifiers(void)
{
	const char *source = "main my_var _private var123 CamelCase";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Identifiers" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_IDENTIFIER, "main", "identifier main");
	assert_token(tokens[2], TOK_IDENTIFIER, "my_var", "identifier my_var");
	assert_token(tokens[4], TOK_IDENTIFIER, "_private", "identifier _private");
	assert_token(tokens[6], TOK_IDENTIFIER, "var123", "identifier var123");
	assert_token(tokens[8], TOK_IDENTIFIER, "CamelCase", "identifier CamelCase");

	lexer_destroy(lexer);
}

/*
 * test_numbers - Test number literals
 */
static void test_numbers(void)
{
	const char *source = "42 0x2A 052 3.14 1e-5 2.5f";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Numbers" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_INTEGER, "42", "decimal integer");
	assert_token(tokens[2], TOK_INTEGER, "0x2A", "hexadecimal");
	assert_token(tokens[4], TOK_INTEGER, "052", "octal");
	assert_token(tokens[6], TOK_FLOAT, "3.14", "float");
	assert_token(tokens[8], TOK_FLOAT, "1e-5", "scientific notation");
	assert_token(tokens[10], TOK_FLOAT, "2.5f", "float with suffix");

	lexer_destroy(lexer);
}

/*
 * test_strings - Test string literals
 */
static void test_strings(void)
{
	const char *source = "\"hello\" \"world\\n\" \"tab\\there\"";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Strings" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_STRING, "\"hello\"", "simple string");
	assert_token(tokens[2], TOK_STRING, "\"world\\n\"", "string with newline escape");
	assert_token(tokens[4], TOK_STRING, "\"tab\\there\"", "string with tab escape");

	lexer_destroy(lexer);
}

/*
 * test_characters - Test character literals
 */
static void test_characters(void)
{
	const char *source = "'a' 'Z' '\\n' '\\t' '\\\\'";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Characters" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_CHAR, "'a'", "char a");
	assert_token(tokens[2], TOK_CHAR, "'Z'", "char Z");
	assert_token(tokens[4], TOK_CHAR, "'\\n'", "char newline");
	assert_token(tokens[6], TOK_CHAR, "'\\t'", "char tab");
	assert_token(tokens[8], TOK_CHAR, "'\\\\'", "char backslash");

	lexer_destroy(lexer);
}

/*
 * test_comments - Test comment handling
 */
static void test_comments(void)
{
	const char *source = "// line comment\n/* block comment */";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Comments" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_COMMENT_LINE, "// line comment",
		     "line comment");
	assert_token(tokens[2], TOK_COMMENT_BLOCK, "/* block comment */",
		     "block comment");

	lexer_destroy(lexer);
}

/*
 * test_preprocessor - Test preprocessor directives
 */
static void test_preprocessor(void)
{
	const char *source = "#include <stdio.h>\n#define MAX 100";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Preprocessor" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_PREPROCESSOR, "#include <stdio.h>",
		     "include directive");
	assert_token(tokens[2], TOK_PREPROCESSOR, "#define MAX 100",
		     "define directive");

	lexer_destroy(lexer);
}

/*
 * test_operators_single - Test single-character operators
 */
static void test_operators_single(void)
{
	const char *source = "+ - * / % = < > ! & | ^ ~ . ? :";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Single-Char Operators" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_PLUS, "+", "plus");
	assert_token(tokens[2], TOK_MINUS, "-", "minus");
	assert_token(tokens[4], TOK_STAR, "*", "star");
	assert_token(tokens[6], TOK_SLASH, "/", "slash");
	assert_token(tokens[8], TOK_PERCENT, "%", "percent");
	assert_token(tokens[10], TOK_ASSIGN, "=", "assign");
	assert_token(tokens[12], TOK_LESS, "<", "less");
	assert_token(tokens[14], TOK_GREATER, ">", "greater");
	assert_token(tokens[16], TOK_LOGICAL_NOT, "!", "not");
	assert_token(tokens[18], TOK_AMPERSAND, "&", "ampersand");
	assert_token(tokens[20], TOK_PIPE, "|", "pipe");
	assert_token(tokens[22], TOK_CARET, "^", "caret");
	assert_token(tokens[24], TOK_TILDE, "~", "tilde");
	assert_token(tokens[26], TOK_DOT, ".", "dot");
	assert_token(tokens[28], TOK_QUESTION, "?", "question");
	assert_token(tokens[30], TOK_COLON, ":", "colon");

	lexer_destroy(lexer);
}

/*
 * test_operators_multi - Test multi-character operators
 */
static void test_operators_multi(void)
{
	const char *source = "== != <= >= && || ++ -- << >> ->";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Multi-Char Operators" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_EQUAL, "==", "equal");
	assert_token(tokens[2], TOK_NOT_EQUAL, "!=", "not equal");
	assert_token(tokens[4], TOK_LESS_EQUAL, "<=", "less equal");
	assert_token(tokens[6], TOK_GREATER_EQUAL, ">=", "greater equal");
	assert_token(tokens[8], TOK_LOGICAL_AND, "&&", "logical and");
	assert_token(tokens[10], TOK_LOGICAL_OR, "||", "logical or");
	assert_token(tokens[12], TOK_INCREMENT, "++", "increment");
	assert_token(tokens[14], TOK_DECREMENT, "--", "decrement");
	assert_token(tokens[16], TOK_LSHIFT, "<<", "left shift");
	assert_token(tokens[18], TOK_RSHIFT, ">>", "right shift");
	assert_token(tokens[20], TOK_ARROW, "->", "arrow");

	lexer_destroy(lexer);
}

/*
 * test_compound_assignment - Test compound assignment operators
 */
static void test_compound_assignment(void)
{
	const char *source = "+= -= *= /= %= &= |= ^= <<= >>=";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Compound Assignment" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_PLUS_ASSIGN, "+=", "plus assign");
	assert_token(tokens[2], TOK_MINUS_ASSIGN, "-=", "minus assign");
	assert_token(tokens[4], TOK_STAR_ASSIGN, "*=", "star assign");
	assert_token(tokens[6], TOK_SLASH_ASSIGN, "/=", "slash assign");
	assert_token(tokens[8], TOK_PERCENT_ASSIGN, "%=", "percent assign");
	assert_token(tokens[10], TOK_AMPERSAND_ASSIGN, "&=", "and assign");
	assert_token(tokens[12], TOK_PIPE_ASSIGN, "|=", "or assign");
	assert_token(tokens[14], TOK_CARET_ASSIGN, "^=", "xor assign");
	assert_token(tokens[16], TOK_LSHIFT_ASSIGN, "<<=", "left shift assign");
	assert_token(tokens[18], TOK_RSHIFT_ASSIGN, ">>=", "right shift assign");

	lexer_destroy(lexer);
}

/*
 * test_punctuation - Test punctuation tokens
 */
static void test_punctuation(void)
{
	const char *source = "( ) { } [ ] ; ,";
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Punctuation" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);

	assert_token(tokens[0], TOK_LPAREN, "(", "left paren");
	assert_token(tokens[2], TOK_RPAREN, ")", "right paren");
	assert_token(tokens[4], TOK_LBRACE, "{", "left brace");
	assert_token(tokens[6], TOK_RBRACE, "}", "right brace");
	assert_token(tokens[8], TOK_LBRACKET, "[", "left bracket");
	assert_token(tokens[10], TOK_RBRACKET, "]", "right bracket");
	assert_token(tokens[12], TOK_SEMICOLON, ";", "semicolon");
	assert_token(tokens[14], TOK_COMMA, ",", "comma");

	lexer_destroy(lexer);
}

/*
 * test_complete_function - Test a complete C function
 */
static void test_complete_function(void)
{
	const char *source = "int main(void)\n{\n\treturn 0;\n}";
	Lexer *lexer;
	Token **tokens;
	int count;

	printf("\n" YELLOW "Testing Complete Function" RESET "\n");

	lexer = lexer_create(source);
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);
	count = lexer_get_token_count(lexer);

	/* Verify we got all expected tokens */
	assert_token(tokens[0], TOK_INT, "int", "function return type");
	assert_token(tokens[2], TOK_IDENTIFIER, "main", "function name");
	assert_token(tokens[3], TOK_LPAREN, "(", "param list start");
	assert_token(tokens[4], TOK_VOID, "void", "void params");
	assert_token(tokens[5], TOK_RPAREN, ")", "param list end");
	assert_token(tokens[7], TOK_LBRACE, "{", "body start");
	assert_token(tokens[10], TOK_RETURN, "return", "return statement");
	assert_token(tokens[12], TOK_INTEGER, "0", "return value");
	assert_token(tokens[15], TOK_RBRACE, "}", "body end");
	assert_token(tokens[count - 1], TOK_EOF, NULL, "end of file");

	lexer_destroy(lexer);
}

/*
 * test_edge_cases - Test edge cases
 */
static void test_edge_cases(void)
{
	Lexer *lexer;
	Token **tokens;

	printf("\n" YELLOW "Testing Edge Cases" RESET "\n");

	/* Empty file */
	lexer = lexer_create("");
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);
	assert_token(tokens[0], TOK_EOF, NULL, "empty file");
	lexer_destroy(lexer);

	/* Only whitespace */
	lexer = lexer_create("   \t\n   ");
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);
	assert_token(tokens[0], TOK_WHITESPACE, "   \t", "whitespace only");
	lexer_destroy(lexer);

	/* Just a number */
	lexer = lexer_create("0");
	lexer_tokenize(lexer);
	tokens = lexer_get_tokens(lexer);
	assert_token(tokens[0], TOK_INTEGER, "0", "single zero");
	lexer_destroy(lexer);
}

/*
 * main - Run all tests
 */
int main(void)
{
	printf("\n");
	printf("╔════════════════════════════════════════════╗\n");
	printf("║      Betty Formatter - Lexer Tests        ║\n");
	printf("╚════════════════════════════════════════════╝\n");

	test_keywords();
	test_identifiers();
	test_numbers();
	test_strings();
	test_characters();
	test_comments();
	test_preprocessor();
	test_operators_single();
	test_operators_multi();
	test_compound_assignment();
	test_punctuation();
	test_complete_function();
	test_edge_cases();

	printf("\n");
	printf("╔════════════════════════════════════════════╗\n");
	printf("║              Test Summary                  ║\n");
	printf("╠════════════════════════════════════════════╣\n");
	printf("║ Total:  %3d tests                          ║\n", tests_run);
	printf("║ " GREEN "Passed: %3d tests" RESET "                          ║\n",
	       tests_passed);

	if (tests_failed > 0)
		printf("║ " RED "Failed: %3d tests" RESET "                          ║\n",
		       tests_failed);
	else
		printf("║ Failed:   0 tests                          ║\n");

	printf("╚════════════════════════════════════════════╝\n");
	printf("\n");

	if (tests_failed > 0)
	{
		printf(RED "❌ Some tests failed\n" RESET);
		return (1);
	}

	printf(GREEN "✅ All tests passed!\n" RESET);
	return (0);
}
