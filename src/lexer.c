#include "compiler.h"

#include "ascii_string.h"

static char peekc(struct lex_process* lexer)
{
	return lexer->functions.peek_char(lexer);
}

static void pushc(struct lex_process* lexer, char c)
{
	lexer->functions.push_char(lexer, c);
}

static char nextc(struct lex_process* lexer)
{
	return lexer->functions.next_char(lexer);
}

void handle_whitespace(struct lex_process* lexer)
{
	struct token* last_token = NULL;
	if (lexer->token_vec.size > 0)
	{
		last_token = vector_get_ptr(lexer->token_vec, lexer->token_vec.size - 1, struct token);
	}

	if (last_token)
	{
		last_token->whitespace_present = 1;
	}

	nextc(lexer);
}

struct token* read_token_number(struct lex_process* lexer)
{
	// Read number string.
	struct string_ascii number_str = string_create_ascii("");
	char c = nextc(lexer);

	for (; c >= '0' && c <= '9'; c = nextc(lexer))
	{
		string_append_char_ascii(&number_str, c);
	}

	if (number_str.length == 0) return NULL; // Next character wasn't the start of a number.

	// Parse string

	long long number = atoll(number_str.str);

	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_NUMBER;
	new_token->value.llnum = number;
	new_token->position = lexer->position;

	return new_token;
}

struct token* read_next_token(struct lex_process* lexer)
{
	struct token* token = NULL;
	char c;
READ_START:

	c = peekc(lexer);
	switch (c)
	{
	CASE_NUMERIC:
		token = read_token_number(lexer);
		printf("Read number token. Token value = %d\n", token->value.llnum);
		break;

	case ' ':
		handle_whitespace(lexer);
		goto READ_START;
		break;

	case(EOF):
		return NULL; // Lexical analysis finished, return no token.
	}

	if (token == NULL)
	{
		lexer->compiler->position = lexer->position;
		compiler_error(lexer->compiler, COMPILER_LEXER_ERROR, LEXER_INPUT_ERROR, "Invalid character '%c' encountered by lexer.\n", c);
	}

	return token;
}

int lex(struct lex_process* lexer)
{
	lexer->current_expression_count = 0;
	lexer->parentheses_buffer = NULL;
	lexer->position.filename = lexer->compiler->input_file.file_abs_path;

	do
	{
		struct token* token = read_next_token(lexer);
		if (!token) break;

		vector_push(lexer->token_vec, *token, struct token);

	} while (1);

	printf("Lexical analysis completed. Token count = %d\n", lexer->token_vec.size);

	return LEXER_ALL_OK;
}