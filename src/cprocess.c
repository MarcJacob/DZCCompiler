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
		out_file = fopen(filename_out, "w");
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