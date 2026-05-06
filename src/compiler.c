#include "compiler.h"

#include <stdarg.h>

void compiler_error(struct compile_process* compiler, int compiler_error_code, int stage_error,
	const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	compiler_error_v(compiler, compiler_error_code, stage_error, msg, args);
	va_end(args);
}

void compiler_error_v(struct compile_process* compiler, int compiler_error_code, int stage_error,
	const char* msg, va_list args)
{
	fprintf(stderr, "\tERROR: ");
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n\t\ton line %i, col %i in file %s\n", compiler->position.line, compiler->position.col, compiler->position.filename);
	
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
		return COMPILER_FATAL_ERROR;
	}

	printf("Compiling file '%s'...\n", filename);

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

	// Copy resulting token vector into compiler's vector.
	compiler->token_vec = vector_create_copy(&lexer->token_vec);

	// Free lexer.
	lex_process_destroy(lexer);
	lexer = NULL;

	// Perform parsing

	// Perform code generation
	
	compile_process_destroy(compiler);
	COMPILER_EXIT_SUCCESS();
}
#undef COMPILER_EXIT_FAILURE