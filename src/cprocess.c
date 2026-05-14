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
char lex_process_next_char(struct lex_process* process);
char lex_process_peek_char(struct lex_process* process);
void lex_process_push_char(struct lex_process* process, char c);

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
	process->functions.next_char = lex_process_next_char;
	process->functions.peek_char = lex_process_peek_char;
	process->functions.push_char = lex_process_push_char;

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

char lex_process_next_char(struct lex_process* process)
{
	struct compile_process* compiler = process->compiler;
	process->position.col++;

	// Get next character in file, and handle newline characters.
	char c = getc(compiler->input_file.file);
	if (c == '\n')
	{
		process->position.line++;
		process->position.col = 1;
	}

	return c;
}

char lex_process_peek_char(struct lex_process* process)
{
	struct compile_process* compiler = process->compiler;
	compiler->position.col++;

	// Peek next character in file, and handle newline characters.
	char c = getc(compiler->input_file.file);
	ungetc(c, compiler->input_file.file);

	return c;
}

void lex_process_push_char(struct lex_process* process, char c)
{
	struct compile_process* compiler = process->compiler;
	ungetc(c, compiler->input_file.file);
}