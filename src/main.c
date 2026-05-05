#include <stdio.h>

#include <process.h>

#include "compiler.h"

#include "ascii_string.h"


int main(int argc, char** argv)
{
	printf("Hello C Compiler written in C ! Let's compile C. Maybe I'm compiling myself. How exciting.\n");

	if (argc == 1)
	{
		printf("Please provide a target file for compilation.\n");
		return 0;
	}

	char* compilation_target_file = argv[1];

	int stage_error;
	int compile_error = compile_file(compilation_target_file, "compile_test_out.exe", NULL, &stage_error);

	system("pause");
	return 0;
}