#include "compiler.h"

#include "ascii_string.h"

static char peekc(struct lex_process* lexer)
{
	return lexer->functions->peek_char(lexer);
}

static void pushc(struct lex_process* lexer, char c)
{
	lexer->functions->push_char(lexer, c);
}

static char nextc(struct lex_process* lexer)
{
	return lexer->functions->next_char(lexer);
}

struct token* read_token_number(struct lex_process* lexer)
{
	// Read number string.
	struct string_ascii number_str = string_create_ascii("");
	char c = peekc(lexer);

	for (; c >= '0' && c <= '9'; c = peekc(lexer))
	{
		string_append_ascii(&number_str, &c);
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
	char c = peekc(lexer);

	switch (c)
	{
	CASE_NUMERIC:
		token = read_token_number(lexer);
		break;

	case(EOF):
		return NULL; // Lexical analysis finished, return no token.
	}

	compiler_error(lexer->compiler, COMPILER_LEXER_ERROR, LEXER_INPUT_ERROR, "Invalid character in file '%s' at on line %d, col %d.", 
		lexer->position.filename, lexer->position.line, lexer->position.col);

	return NULL;
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

	return LEXER_ALL_OK;
}