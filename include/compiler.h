#ifndef COMPILER_INCLUDED
#define COMPILER_INCLUDED

#include <stdio.h>

#include "core_types.h"
#include "vector.h"

struct token_position
{
	int line;
	int col;
	const char* filename;
};

enum
{
	TOKEN_TYPE_IDENTIFIER,
	TOKEN_TYPE_KEYWORD,
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_SYMBOL,
	TOKEN_TYPE_NUMBER,
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_COMMENT,
	TOKEN_TYPE_NEWLINE,
};

// Value associated to a token, can be one of many types. See the token structure.
union token_value
{
	char cval;
	const char* strval;
	ui32 inum;
	ui64 lnum;
	ui64 llnum;
	void* any;
};

// Result of lexical analysis, associating a type, a value and secondary properties.
struct token
{
	union token_value value;
	struct token_position position;

	// Pointer to start of closest opening parenthesis or brackets.
	const char* enclosure;

	// Type of token, corresponds to token_type enumeration.
	int type;
	int flags;

	// Is at least one white space present between this token and the next one ?
	byte whitespace_present;
};

// Return values for the compile_file function.
enum
{
	COMPILER_FILE_COMPILED_OK,
	COMPILER_FAILED_WITH_ERRORS,
};

struct lex_process;

typedef char (*LEX_PROCESS_NEXT_CHAR_FUNC)(struct lex_process* process);
typedef char (*LEX_PROCESS_PEEK_CHAR_FUNC)(struct lex_process* process);
typedef void (*LEX_PROCESS_PUSH_CHAR_FUNC)(struct lex_process* process, char c);

struct lex_process_functions
{
	LEX_PROCESS_NEXT_CHAR_FUNC next_char;
	LEX_PROCESS_PEEK_CHAR_FUNC peek_char;
	LEX_PROCESS_PUSH_CHAR_FUNC push_char;
};

struct lex_process
{
	struct token_position position;
	struct vector* token_vec;
	struct compile_process* compiler;

	// How many brackets / parenthesis are open at the current position.
	int current_expression_count;
	byte* parentheses_buffer;

	struct lex_process_functions* functions;

	// Private data the user of the lexer can transfer to the process.
	void* private;
};

struct compile_process_input_file
{
	FILE* file;
	const char* file_abs_path;
};

// Ongoing compilation process.
struct compile_process
{
	// Flags in regards to how this file should be compiled.
	int flags;

	// Input file structure containing the FILE handle and its absolute path.
	struct compile_process_input_file input_file;

	FILE* output_file;
};

struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);
void compile_process_destroy(struct compile_process* process);

int compile_file(const char* filename, const char* out_filename, int flags);

#endif // COMPILER_INCLUDED