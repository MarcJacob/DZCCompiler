#ifndef COMPILER_INCLUDED
#define COMPILER_INCLUDED

#include <stdio.h>

#include "core_types.h"
#include "vector.h"
#include "ascii_string.h"

struct token_position
{
	int line;
	int col;
	const char* filename;
};

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
	struct string_ascii strval;
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

// Return values for the lex function.
enum
{
	LEXER_ALL_OK,
	LEXER_FATAL_ERROR,
	LEXER_INPUT_ERROR,
};

// Return values for the compile_file function.
enum
{
	COMPILER_FILE_COMPILED_OK, // Compilation successful.
	COMPILER_FATAL_ERROR, // Fatal error in the compiler process itself, not due to user input.
	COMPILER_FAILED_WITH_ERRORS, // Compilation error of any kind (check this or above for "normal" errors related to user input).
	COMPILER_LEXER_ERROR, // Failed at the Lexical Analysis stage.
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
	struct vector token_vec;
	struct compile_process* compiler;

	// How many brackets / parenthesis are open at the current position.
	int current_expression_count;
	byte* parentheses_buffer;

	struct lex_process_functions functions;

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

	// Current position of the compilation process.
	struct token_position position;

	// Input file structure containing the FILE handle and its absolute path.
	struct compile_process_input_file input_file;

	FILE* output_file;

	// Output Status

	// COMPILER_ERROR enum value, indicating an error in the compiler process itself or which stage of the process failed.
	int compiler_error;

	// If compiler_error indicates an error in a specific stage, this contains an error code relevant to that stage.
	int stage_error;
};

struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);
void compile_process_destroy(struct compile_process* process);

// Emits an error message in the standard error stream and populates the error codes in the compiler process structure.
// The compilation process should obviously stop and return as soon as possible after this.
void compiler_error(struct compile_process* compiler, int compiler_error_code, int stage_error, const char* msg, ...);

struct lex_process* lex_process_create(struct compile_process* compiler, void* private_data);
void lex_process_destroy(struct lex_process* lexer);

// Start compilation for a specific file. Returns one of the COMPILER_* enumeration values.
// If failure happened inside one of the stages, will return a general enum value indicating which stage,
// and out_stage_error will be populated with the stage-specific error code.
int compile_file(const char* filename, const char* out_filename, int flags, int* out_stage_error);

int lex(struct lex_process* lexer);

#endif // COMPILER_INCLUDED