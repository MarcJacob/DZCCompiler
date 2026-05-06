#include <stdarg.h>

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

static inline void lexer_error(struct lex_process* lexer, int lexer_error, const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	lexer->compiler->position = lexer->position;
	compiler_error_v(lexer->compiler, COMPILER_LEXER_ERROR, lexer_error, msg, args);

	va_end(args);
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

	printf("Read number token. Token value = %I64d\n", new_token->value.llnum);
	return new_token;
}

struct token* read_token_string(struct lex_process* lexer, char delim)
{
	// Read character string starting with a "
	struct string_ascii string = string_create_ascii("");

	if (nextc(lexer) != delim)
	{
		lexer_error(lexer, LEXER_INPUT_ERROR, "Attempted to read String token with wrong start character.");
		return NULL;
	}

	char c = nextc(lexer);
	for (; c != delim && c != EOF; c = nextc(lexer))
	{
		if (c == '\\')
		{
			// TODO: Handle escape.
			continue;
		}

		string_append_char_ascii(&string, c);

		// Peek and pre-handle error characters:
		c = peekc(lexer);
		if (c == EOF)
		{
			lexer_error(lexer, LEXER_INPUT_ERROR, "Encountered EOF when reading String token.");
			return NULL;
		}	
	}

	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_STRING;
	new_token->value.strval = string;
	new_token->position = lexer->position;

	printf("Read string token. Token value = %c%s%c\n", delim, new_token->value.strval.str, delim);
	return new_token;
}

struct token* read_newline_token(struct lex_process* lexer)
{
	char c = peekc(lexer);
	if (c != '\n')
	{
		lexer_error(lexer, LEXER_INPUT_ERROR, "Attempted to read '%c' as new line token.", c);
		return NULL;
	}

	nextc(lexer);

	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_NEWLINE;
	new_token->value.cval = '\n';
	new_token->position = lexer->position;

	printf("Read newline token.\n");

	return new_token;
}

struct token* read_token_comment(struct lex_process* lexer, int multiline)
{
	// It is expected that the current position of the lexer is before the first character in the comment,
	// meaning the comment start characters ("//" or "/*") have already been read.

	char c = nextc(lexer);
	char c2 = peekc(lexer);

	struct string_ascii comment_str = string_create_ascii("");

	// Begin reading the comment. Read the characters two by two, c reading the file while c2 peeks it.
	// This means that, effectively, c2 of iteration N becomes c of iteration N+1 and so on.
	// Stop at EOF no matter what. If multiline, stop when reading c == '*' and c2 == '/'. Otherwise stop at line break.
	for (;	(multiline && (c != '*' || c2 != '/')) 
			||	(!multiline && c != '\n');

			c = nextc(lexer), c2 = peekc(lexer))
	{
		if (c == EOF) break; // TODO: Disallow multiline comments that end at EOF instead of */ ?

		// Only add c to the token's string content. c2 is only there to "catch" */ before the asterisk gets added.
		string_append_char_ascii(&comment_str, c);
	}

	if (multiline && c2 == '/') c2 = nextc(lexer); // Special case: multiline comment ends with two characters which want to have read, so advance by one more character now.

	// Empty comments are allowed so as soon as this function is entered, it always returns something.
	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_COMMENT;
	new_token->value.strval = comment_str;
	new_token->flags |= (TOKEN_FLAG_MULTILINE * multiline);
	new_token->position = lexer->position;

	printf("Read comment token (multiline = %d). Token value = %s\n", multiline, comment_str.str);

	return new_token;
}

struct token* read_next_token(struct lex_process* lexer)
{
	struct token* token = NULL;
	char c, c2; // c2 is useful for "simple" token types that have / start with two characters.
READ_START:

	c = peekc(lexer);
	switch (c)
	{
	CASE_NUMERIC:
		token = read_token_number(lexer);
		break;
	case '"':
		token = read_token_string(lexer, '"');
		break;
	case '\'':
		token = read_token_string(lexer, '\'');
		break;
	case ' ':
		handle_whitespace(lexer);
		goto READ_START;
	case '\n':
		token = read_newline_token(lexer);
		break;
	case '/':
		// Determine if this is the start of a comment.
		// If next char is another slash or an asterisk, it is a comment.
		// Otherwise, it is one of the complex token types.
		
		c = nextc(lexer);
		c2 = peekc(lexer);

		switch (c2)
		{
		case('/'):
			c2 = nextc(lexer);
			token = read_token_comment(lexer, 0);
			break;
		case('*'):
			c2 = nextc(lexer);
			token = read_token_comment(lexer, 1);
			break;
		default:
			// Not a comment. Put c back and pretend nothing happened.
			pushc(lexer, c);
			break;
		}

		// If we failed to get a comment token from this character, go to the unhandled block.
		if (!token) goto LEX_UNHANDLED_CHARACTER;
		break;

	case(EOF):
		return NULL; // Lexical analysis finished, return no token.

	default:
	LEX_UNHANDLED_CHARACTER:
		// Unhandled character, error out.
		// TODO: Handle more complex tokens. 
		lexer_error(lexer, LEXER_INPUT_ERROR, "Invalid character '%c' encountered by lexer.", c);
		return NULL;
	}

	return token;
}

int lex(struct lex_process* lexer)
{
	lexer->current_expression_count = 0;
	lexer->parentheses_buffer = NULL;
	lexer->position.filename = lexer->compiler->input_file.file_abs_path;

	// Read as many tokens as possible.
	do
	{
		struct token* token = read_next_token(lexer);
		if (!token) break;

		vector_push(lexer->token_vec, *token, struct token);

	} while (1);

	// Exit if any error happened during the token reading process.
	if (compiler_has_error(lexer->compiler))
	{
		return lexer->compiler->stage_error;
	}

	printf("Lexical analysis completed. Token count = %d\n", lexer->token_vec.size);

	return LEXER_ALL_OK;
}