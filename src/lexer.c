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

struct token* read_token_newline(struct lex_process* lexer)
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

struct token* read_token_comment(struct lex_process* lexer)
{
	char c = nextc(lexer); // Should be '/' per the execution condition of this function.
	char c2 = peekc(lexer); // Should be '/' or '*' per the execution condition of this function.

	if (c2 != '/' && c2 != '*')
	{
		// Not a comment token. Push the c character back in and return NULL.
		pushc(lexer, c);
		return NULL;
	}

	struct string_ascii comment_str = string_create_ascii("");

	int multiline = c2 == '*';

	// Otherwise, advance past c2 properly so it doesn't get added to the comment string itself.
	nextc(lexer);
	c = nextc(lexer);
	c2 = peekc(lexer);

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

struct token* read_token_operator(struct lex_process* lexer)
{
	// pre-allocate token and fill in the fields we can fill in.
	struct token* new_token = calloc(1, sizeof(struct token));
	new_token->type = TOKEN_TYPE_OPERATOR;
	
	char c = nextc(lexer);
	char c2 = peekc(lexer);

	// Determine str value of operator token. If no value is assigned, we failed to find an operator token from the characters.
	const char* op_val = NULL;
	switch (c)
	{
	case('+'): // Add, Increment or Add-Assignment ?
		if (c2 == '+')
		{
			c2 = nextc(lexer);
			op_val = "++";
		}
		else if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = "+=";
		}
		else
		{
			op_val = "+";
		}
		break;
	case('-'): // Subtract, Decrement or Subtract-Assignment ?
		if (c2 == '-')
		{
			c2 = nextc(lexer);
			op_val = "--";
		}
		else if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = "-=";
		}
		else
		{
			op_val = "-";
		}
		break;
	case('*'): // Multiply, Multiply-Assignment, pointer dereferencing or pointer type ?
		if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = "*=";
		}
		else
		{
			op_val = "*";
		}
		break;
	case('>'): // Greater, Greater equal or Bitshift Right ?
		if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = ">=";
		}
		else if (c2 == '>')
		{
			c2 = nextc(lexer);
			op_val = ">>";
		}
		else
		{
			op_val = ">";
		}
		break;
	case('<'): // Lower, Lower equal or Bitshift Left ?	
		if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = "<=";
		}
		else if (c2 == '<')
		{
			c2 = nextc(lexer);
			op_val = "<<";
		}
		else
		{
			op_val = "<";
		}
		break;
	case('&'): // Bitwise AND or AND operator ?
		if (c2 == '&')
		{
			c2 = nextc(lexer);
			op_val = "&&";
		}
		else
		{
			op_val = "&";
		}
		break;
	case('|'): // Bitwise OR or OR operator ?
		if (c2 == '|')
		{
			c2 = nextc(lexer);
			op_val = "||";
		}
		else
		{
			op_val = "|";
		}
		break;
	}

	// Found a value for the operator ! Store it as a string. TODO: New string allocated everytime even though we could just have the string act as a "view" in this case.
	if (op_val != NULL)
	{
		new_token->value.strval = string_create_ascii(op_val);
		new_token->position = lexer->position;

		printf("Read Operator Token. Token value = %s\n", new_token->value.strval.str);
		return new_token;
	}

	// Failed to read any operator in. Push c character back and return NULL.
	pushc(lexer, c);
	return NULL;
}

struct token* read_next_token(struct lex_process* lexer)
{
	struct token* token = NULL;
	char c;

TOKEN_READ_START:
	c = peekc(lexer);

	// Handle the "Simple" tokens which are tokens that invariably start with the same one or two characters with no regard to context.
	// They include Numbers, Strings, whitespaces (added to the latest token if any), comments, newlines and EOF.

SIMPLE_TOKEN_READ_START:

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
		goto TOKEN_READ_START; // Jump back to beginning of read to handle next character.
	case '\n':
		token = read_token_newline(lexer);
		break;
	case '/':
		// Attempt to read a comment token. The function can fail in which case token will be NULL.
		token = read_token_comment(lexer);
		break;

	case(EOF):
		return NULL; // Lexical analysis finished, return no token.
	}

	// Return "simple" token if found.
	if (token)
	{
		return token;
	}

	// Otherwise, continue on to operator tokens.
	// Operators are made up of one or two characters whose exact meaning is contextual, so all the lexer has to do is determine
	// the correct string value for the token and let the parser do the work of figuring out the exact operator type.
OPERATOR_TOKEN_READ_START:

	// Return operator token if found.
	token = read_token_operator(lexer);
	if (token) return token;

LEX_UNHANDLED_CHARACTER:
	// Unhandled character, emit an error and return no token.
	lexer_error(lexer, LEXER_INPUT_ERROR, "Invalid character '%c' encountered by lexer; Next token type cannot be inferred.", c);
	return NULL;
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