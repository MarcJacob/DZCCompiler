#include "compiler.h"

#include <stdarg.h>

void compiler_error(struct compile_process* compiler, int compiler_error_code, int stage_error,
	const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	vfprintf(stderr, msg, args);
	va_end(args);

	fprintf(stderr, " on line %i, col %i in file %s\n", compiler->position.line, compiler->position.col, compiler->position.filename);
	
	compiler->compiler_error = compiler_error_code;
	compiler->stage_error = stage_error;
}

#define COMPILER_EXIT_FAILURE() *out_stage_error = compiler->stage_error; return compiler->compiler_error;
#define COMPILER_EXIT_SUCCESS() return 0;
int compile_file(const char* filename, const char* out_filename, int flags, int* out_stage_error)
{
	struct compile_process* compiler = compile_process_create(filename, out_filename, flags);
	if (!compiler)
	{
		// Can't use exit macro since the compiler itself failed to be created.
		fprintf(stderr, "Failed to create compile process !");
		return COMPILER_FATAL_ERROR;
	}

	// Perform lexical analysis.

	// Create lexer process.
	struct lex_process* lexer = lex_process_create(compiler, NULL);
	if (!lexer)
	{
		COMPILER_EXIT_FAILURE();
	}

	// Run it.
	if (lex(lexer) != LEXER_ALL_OK)
	{
		COMPILER_EXIT_FAILURE();
	}

	// Perform parsing

	// Perform code generation
	
	lex_process_destroy(lexer);
	compile_process_destroy(compiler);
	COMPILER_EXIT_SUCCESS();
}
#undef COMPILER_EXIT_FAILURE