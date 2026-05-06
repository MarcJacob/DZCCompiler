#include <stdarg.h>

#include "compiler.h"
#include "ascii_string.h"

#define CASE_NUMERIC	\
	case '0':			\
	case '1':			\
	case '2':			\
	case '3':			\
	case '4':			\
	case '5':			\
	case '6':			\
	case '7':			\
	case '8':			\
	case '9'
	

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

int get_keyword_index(const char* keyword)
{
	const int keywords_table_size = sizeof(KEYWORDS_STR_TABLE) / sizeof(char*);
	for (int keyword_index = 0; keyword_index < keywords_table_size; keyword_index++)
	{
		if (strcmp(keyword, KEYWORDS_STR_TABLE[keyword_index]) == 0)
		{
			return keyword_index;
		}
	}

	return -1;
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
	// TODO: Handle Hexadecimal and Float / Double literals.

	// Read number string.
	struct string_ascii number_str = string_create_ascii("");
	char c = peekc(lexer);

	for (; c >= '0' && c <= '9'; c = peekc(lexer))
	{
		string_append_char_ascii(&number_str, c);
		nextc(lexer);
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
			// Peek next character and handle escaped characters.
			char next_c = peekc(lexer);
			switch (next_c)
			{
			case 'n':
				c = '\n';
				break;
			case '0':
				c = '\0';
				break;
			case 't':
				c = '\t';
				break;
			case '\'':
				c = '\'';
				break;
			case '\"':
				c = '"';
				break;
			case '\\':
				c = '\\';
				break;
			}

			nextc(lexer); // Advance an extra character.
		}

		string_append_char_ascii(&string, c);
	}

	if (c == EOF)
	{
		lexer_error(lexer, LEXER_INPUT_ERROR, "Encountered EOF when reading String token.");
		return NULL;
	}	

	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_STRING;
	new_token->value.strval = string;
	new_token->position = lexer->position;

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
		if (c == EOF)
		{
			// If comment is multiline, then this is an error case since multiline comments HAVE to be closed !
			if (multiline)
			{
				lexer_error(lexer, LEXER_INPUT_ERROR, "Reached end of file before closing multiline comment. Use '*\\' to close the comment.");
				string_free_ascii(&comment_str);
				return NULL;
			}

			break;
		}

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

	return new_token;
}

struct token* read_token_operator(struct lex_process* lexer)
{
	// Pre-allocate token and fill in the fields we can fill in.
	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

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
	case('-'): // Subtract, Decrement, Subtract-Assignment or Dereferenced Structured Access ?
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
		else if (c2 == '>')
		{
			c2 = nextc(lexer);
			op_val = "->";
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
	case('/'): // Division or Divide-Assignment
		if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = "/=";
		}
		else
		{
			op_val = "/";
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
	case('='): // Assignment or Equal operator ?
		if (c2 == '=')
		{
			c2 = nextc(lexer);
			op_val = "==";
		}
		else
		{
			op_val = "=";
		}
		break;
	case('.'): // Structured access
		op_val = ".";
		break;
	case(','): // Parameters delimiter
		op_val = ",";
		break;
	}

	// Found a value for the operator ! Store it as a string. TODO: New string allocated everytime even though we could just have the string act as a "view" in this case.
	if (op_val != NULL)
	{
		new_token->value.strval = string_create_ascii(op_val);
		new_token->position = lexer->position;

		return new_token;
	}

	// Failed to read any operator in. Free token, push c character back and return NULL.
	pushc(lexer, c);
	free(new_token);
	return NULL;
}

struct token* read_token_symbol(struct lex_process* lexer)
{
	// Pre-allocate token and fill in the fields we can fill in.
	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_SYMBOL;
	
	char c = nextc(lexer);

	// Determine str value of symbol token. If no value is assigned, we failed to find a symbol token from the characters.
	const char* symbol_val = NULL;
	switch (c)
	{
	case('('): // Expression / Exec Operator scope begin
		symbol_val = "(";
		break;
	case(')'): // Expression / Exec Operator scope end
		symbol_val = ")";
		break;
	case('['): // Index expression begin
		symbol_val = "[";
		break;
	case(']'): // Index expression end
		symbol_val = "]";
		break;
	case('{'): // Block scope begin
		symbol_val = "{";
		break;
	case('}'): // Block scope end
		symbol_val = "}";
		break;
	case(';'): // Statement end
		symbol_val = ";";
		break;
	case(':'): // Label locator end
		symbol_val = ":";
		break;
	}

	// If a symbol value was found, return the token.
	if (symbol_val != NULL)
	{
		new_token->value.strval = string_create_ascii(symbol_val);
		new_token->position = lexer->position;

		return new_token;
	}

	// ... Otherwise, free the token, push the character back and return NULL.
	pushc(lexer, c);
	free(new_token);
	return NULL;
}

int is_word_char(char c, int can_be_number)
{
	return c == '_' // Underscore
		|| (c >= 'a' && c <= 'z') // Lower case letter
		|| (c >= 'A' && c <= 'Z') // Upper case letter
		|| (can_be_number && c >= '0' && c <= '9');
}

// Reads a "word" which starts with a letter or underscore and ends with a alphanumeric or underscore.
// Words are used for keywords and identifiers. Returns a new string containing the word and advances the lexer to after the word.
// Note: if returned string is empty, the current lexer position wasn't a valid start for a word.
struct string_ascii read_word(struct lex_process* lexer)
{
	struct string_ascii word_str = string_create_ascii("");

	char c = peekc(lexer);

	// Check validity of first char as a word starter.
	if (!is_word_char(c, 0))
	{
		return word_str;
	}

	// Read and append all following characters so long as they are valid word characters.
	for (; is_word_char(c, 1); c = peekc(lexer))
	{
		string_append_char_ascii(&word_str, c);
		nextc(lexer);
	}

	return word_str;
}

struct token* read_token_keyword(struct lex_process* lexer, struct string_ascii* word)
{
	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	int keyword_index = get_keyword_index(word->str);
	if (keyword_index >= 0)
	{
		new_token->type = TOKEN_TYPE_KEYWORD;
		new_token->value.keyword_index = keyword_index;
		new_token->position = lexer->position;

		return new_token;
	}

	// Word didn't correspond to a keyword. Free the token and return NULL.
	free(new_token);
	return NULL;
}

struct token* read_token_identifier(struct lex_process* lexer, struct string_ascii* word)
{
	// We know the word is non-empty and only contains characters allowed in words, so anything goes.
	struct token* new_token = calloc(1, sizeof(struct token));
	if (!new_token) abort();

	new_token->type = TOKEN_TYPE_IDENTIFIER;
	new_token->value.strval = *word;
	new_token->position = lexer->position;

	return new_token;
}

struct token* read_next_token(struct lex_process* lexer)
{
	struct token* token = NULL;
	char c;

TOKEN_READ_START:
	c = peekc(lexer);

	// Handle the "Simple" tokens which are tokens that invariably start with the same one or two characters with no regard to context.
	// They include Numbers, Strings, whitespaces (added to the latest token if any), comments, newlines and EOF.

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
	case '\t': // For now let's treat tabs and whitespaces the same.
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
	// Otherwise check for errors.
	else if (compiler_has_error(lexer->compiler)) return NULL;

	// Otherwise, continue on to operator tokens.
	// Operators are made up of one or two characters whose exact meaning is contextual, so all the lexer has to do is determine
	// the correct string value for the token and let the parser do the work of figuring out the exact operator type.

	// Return operator token if found.
	token = read_token_operator(lexer);
	if (token) return token;
	else if (compiler_has_error(lexer->compiler)) return NULL;

	// Return symbol token if found.
	token = read_token_symbol(lexer);
	if (token) return token;
	else if (compiler_has_error(lexer->compiler)) return NULL;

	// Word reading: read a word in, then try to read it first as a keyword, then an identifier.
	struct string_ascii word = read_word(lexer);
	if (word.length > 0)
	{
		token = read_token_keyword(lexer, &word);
		if (token)
		{
			// Keyword token was read. Keywords keep a keyword index, not a string, so the word string can be freed.
			string_free_ascii(&word);
		}
		else if (compiler_has_error(lexer->compiler)) return NULL;
		// If reading a keyword failed, read the word as an identifier.
		else token = read_token_identifier(lexer, &word);

		if (compiler_has_error(lexer->compiler)) return NULL;

		// Reading an identifier cannot fail provided the word is non-empty, so token will always have a value here.
		return token;
	}
	else
	{
		// No word was found - free the string.
		string_free_ascii(&word);
	}

	// Unhandled character, emit an error and return no token.
	lexer_error(lexer, LEXER_INPUT_ERROR, "Invalid character '%c' encountered by lexer. Next token type cannot be inferred.", c);
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

		vector_push(lexer->token_vec, *token);
		free(token); // Free token pointer. It is not needed anymore since we've "moved" it into the vector token.
	} while (1);

	// Exit if any error happened during the token reading process.
	if (compiler_has_error(lexer->compiler))
	{
		return lexer->compiler->stage_error;
	}

	// Print read tokens.
	for (int token_index = 0; token_index < lexer->token_vec.size; token_index++)
	{
		struct token* lex_token = vector_get_ptr(lexer->token_vec, token_index, struct token);
		switch (lex_token->type)
		{
		case TOKEN_TYPE_NUMBER:
			printf("NUMBER\t%lld\n", lex_token->value.llnum);
			break;
		case TOKEN_TYPE_STRING:
			printf("STRING\t\"%s\"\n", lex_token->value.strval.str);
			break;
		case TOKEN_TYPE_COMMENT:
			printf("COMMENT\t%s\n", lex_token->value.strval.str);
			break;
		case TOKEN_TYPE_NEWLINE:
			printf("NEWLINE\t\n");
			break;
		case TOKEN_TYPE_OPERATOR:
			printf("OP\t%s\n", lex_token->value.strval.str);
			break;
		case TOKEN_TYPE_SYMBOL:
			printf("SYMBOL\t%s\n", lex_token->value.strval.str);
			break;
		case TOKEN_TYPE_KEYWORD:
			printf("KEYWORD\t%d ('%s')\n", lex_token->value.keyword_index, KEYWORDS_STR_TABLE[lex_token->value.keyword_index]);
			break;
		case TOKEN_TYPE_IDENTIFIER:
			printf("IDENT\t%s\n", lex_token->value.strval.str);
			break;
		}

		if (lex_token->whitespace_present)
		{
			printf("Whitespace\n");
		}

	}

	printf("\nLexical analysis completed. Token count = %d\n", lexer->token_vec.size);

	return LEXER_ALL_OK;
}