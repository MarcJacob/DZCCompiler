#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"

struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags)
{
	FILE* file = fopen(filename, "r");

	if (!file) return NULL;

	FILE* out_file = NULL;
	if (filename_out)
	{
		fopen_s(&out_file, filename_out, "w");
		if (!out_file) return NULL;
	}

	// Init compile process structure.
	struct compile_process* process = calloc(1, sizeof(struct compile_process));
	if (!process) return NULL; // TODO: Assertion system.

	process->flags = flags;
	process->input_file.file = file;
	process->input_file.file_abs_path = filename;
	process->output_file = out_file;

	return process;
}

void compile_process_destroy(struct compile_process* process)
{
	free(process);
}

// Default lexer functions.
char compile_process_next_char(struct lex_process* process);
char compile_process_peek_char(struct lex_process* process);
void compile_process_push_char(struct lex_process* process, char c);

struct lex_process* lex_process_create(struct compile_process* compiler, void* private_data)
{
	struct lex_process* process = calloc(1, sizeof(struct lex_process));
	if (!process)
	{
		compiler_error(compiler, COMPILER_LEXER_ERROR, LEXER_FATAL_ERROR, "Failed to allocate Lexer process !\n");
		return NULL;
	}

	process->compiler = compiler;
	process->private = private_data;

	// Initialize token vector and parentheses buffer.
	process->token_vec = vector_create(struct token, 256);
	process->parentheses_buffer = calloc(256, 1);

	// Init functions to defaults.
	process->functions->next_char = compile_process_next_char;
	process->functions->peek_char = compile_process_peek_char;
	process->functions->push_char = compile_process_push_char;

	process->position.col = 1;
	process->position.line = 1;
	process->position.filename = compiler->input_file.file_abs_path;

	return process;
}

void lex_process_destroy(struct lex_process* lexer)
{
	vector_free_ptr(&lexer->token_vec);
	free(lexer->parentheses_buffer);
	free(lexer);
}

char compile_process_next_char(struct lex_process* process)
{
	struct compile_process* compiler = process->compiler;
	compiler->position.col++;

	// Get next character in file, and handle newline characters.
	char c = getc(compiler->input_file.file);
	if (c == '\n')
	{
		compiler->position.line++;
		compiler->position.col = 1;
	}

	return c;
}

char compile_process_peek_char(struct lex_process* process)
{
	struct compile_process* compiler = process->compiler;
	compiler->position.col++;

	// Peek next character in file, and handle newline characters.
	char c = getc(compiler->input_file.file);
	ungetc(c, compiler->input_file.file);

	return c;
}

void compile_process_push_char(struct lex_process* process, char c)
{
	struct compile_process* compiler = process->compiler;
	ungetc(c, compiler->input_file.file);
}