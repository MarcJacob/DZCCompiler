#include "compiler.h"

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

struct token* read_next_token(struct lex_process* lexer)
{
	struct token* token = NULL;
	char c = peekc(lexer);

	switch (c)
	{
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