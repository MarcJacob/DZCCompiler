#include <stdio.h>

#include <process.h>

#include "compiler.h"

#include "ascii_string.h"


int main(int argc, char** argv)
{
	printf("Hello C Compiler written in C ! Let's compile C. Maybe I'm compiling myself. How exciting.\n");

	const char* input_filename = NULL;
	if (argc == 1)
	{
		printf("Please provide a target file for compilation.\n");
		char input_buffer[256];
		char* input = gets_s(input_buffer, sizeof(input_buffer));

		if (!input)
		{
			printf("Invalid input.\n");
			goto PROGRAM_END;
		}
	
		input_filename = input;
	}

	int stage_error;
	int compile_error = compile_file(input_filename, "compile_test_out.exe", 0, &stage_error);

PROGRAM_END:
	system("pause");
	return 0;
}