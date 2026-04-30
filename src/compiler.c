#include "compiler.h"

int compile_file(const char* filename, const char* out_filename, int flags, int* out_stage_error)
{
	struct compile_process* compiler = compile_process_create(filename, out_filename, flags);
	if (!compiler)
	{
		return COMPILER_FAILED_WITH_ERRORS;
	}

	// Perform lexical analysis
	struct lex_process* lexer = lex_process_create(compiler, NULL);
	if (!lexer)
	{
		return COMPILER_FATAL_ERROR;
	}

	if ((*out_stage_error = lex(lexer)) != LEXER_ALL_OK)
	{
		return COMPILER_LEXER_ERROR;
	}

	// Perform parsing

	// Perform code generation
	
	lex_process_destroy(lexer);
	compile_process_destroy(compiler);
	return COMPILER_FILE_COMPILED_OK;
}