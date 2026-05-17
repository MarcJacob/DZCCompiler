#include <stdio.h>
#include <process.h>

#include "compiler.h"
#include "ascii_string.h"

// Directly include major compiler step implementations.

#include "cprocess.c"
#include "compiler.c"
#include "lexer/lexer.c"
#include "parser/parser.c"
#include "symbol_resolution/symbol_resolution.c"

int main(int argc, char** argv)
{
	printf("Hello C Compiler written in C ! Let's compile C. Maybe I'm compiling myself. How exciting.\n");

	const char* input_filename = NULL;
PROGRAM_START:
	if (argc == 1)
	{
		printf("Enter input file name: ");
		char input_buffer[256];
		char* input = gets_s(input_buffer, sizeof(input_buffer));

		if (!input || strlen(input) == 0)

		{
			printf("Invalid input.\n");
			goto PROGRAM_START;
		}
	
		input_filename = input;
	}
	else
	{
		input_filename = argv[1];
	}

	int stage_error;
	int compile_error = compile_file(input_filename, "compile_test_out.exe", 0, &stage_error);

	// If in "interactive" mode (no CLA), loop back to start if there was an error.
	if (argc == 1 && compile_error != COMPILER_FILE_COMPILED_OK)
	{
		printf("Compiler process ended with errors.\n\n");
		goto PROGRAM_START;
	}

	system("pause");
	return 0;
}