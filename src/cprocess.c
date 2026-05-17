#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"

struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags)
{
	FILE* file = fopen(filename, "r");

	if (!file)
	{
		fprintf(stderr, "\tFATAL ERROR: Could not find input file '%s'.\n", filename);
		return NULL;
	}

	FILE* out_file = NULL;
	if (filename_out)
	{
		fopen_s(&out_file, filename_out, "w");
			}

	// Init compile process structure.
	struct compile_process* process = calloc(1, sizeof(struct compile_process));
	if (!process) return NULL; // TODO: Assertion system.

	process->flags = flags;
	process->input_file.file = file;
	process->input_file.file_abs_path = filename;
	process->output_file = out_file;

	process->node_vec = vector_create(struct parsing_node, 64);
	process->node_tree_vec = vector_create(struct parsing_node*, 64);

	process->scopes = vector_create(struct scope, 32);

	// Create Global Scope.
	struct scope global_scope = { 0 };
	global_scope.parent = NULL;
	global_scope.flags = 0;
	global_scope.symbols = vector_create(struct compiled_symbol, 64);

	vector_push(process->scopes, global_scope);
	return process;
}

void compile_process_destroy(struct compile_process* process)
{
	if (process->input_file.file)
	{
		fclose(process->input_file.file);
	}
	free(process);
}

// Default lexer functions.

// Default next_char implementation, assuming the private data pointer points to a FILE handle.
char lex_process_next_char_default(struct lex_process* process);
// Default peek_char implementation, assuming the private data pointer points to a FILE handle.
char lex_process_peek_char_default(struct lex_process* process);
// Default push_char implementation, assuming the private data pointer points to a FILE handle.
void lex_process_push_char_default(struct lex_process* process, char c);

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

	// Init functions to defaults.
	process->functions.next_char = lex_process_next_char_default;
	process->functions.peek_char = lex_process_peek_char_default;
	process->functions.push_char = lex_process_push_char_default;

	process->position.col = 1;
	process->position.line = 1;
	process->position.filename = compiler->input_file.file_abs_path;

	return process;
}

void lex_process_destroy(struct lex_process* lexer)
{
	vector_free_ptr(&lexer->token_vec);
	free(lexer);
}

char lex_process_next_char_default(struct lex_process* process)
{
	assert(process->private != NULL);
	FILE* source_file = (FILE*)process->private;

	process->position.col++;

	// Get next character in file, and handle newline characters.
	char c = (char)getc(source_file);
	if (c == '\n')
	{
		process->position.line++;
		process->position.col = 1;
	}

	return c;
}

char lex_process_peek_char_default(struct lex_process* process)
{
	assert(process->private != NULL);
	FILE* source_file = (FILE*)process->private;

	// Peek next character in file, and handle newline characters.
	char c = (char)getc(source_file);
	ungetc(c, source_file);

	return c;
}

void lex_process_push_char_default(struct lex_process* process, char c)
{
	assert(process->private != NULL);
	FILE* source_file = (FILE*)process->private;

	// NOTE: We don't decrement file position here, so we might get an offset the more pushes happen.
	// It's not as simple as decrementing the column because we then need to be able to go back to previous line's end, so I need
	// to come up with a decent solution for caching the previous line's length.

	ungetc(c, source_file);
}