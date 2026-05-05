#include <stdio.h>

#include <process.h>

#include "compiler.h"

#include "ascii_string.h"


int main(int argc, char** argv)
{
	printf("Hello C Compiler written in C ! Let's compile C. Maybe I'm compiling myself. How exciting.\n");

	// Narrator: It doesn't actually compile or really do anything right now.

	struct string_ascii test = string_create_ascii("Hello World !\n");

	int test_len = (int)strlen(test.str);

	printf(test.str);
	printf("Length = %d. test[%d] = %c\n", test_len, test_len, test.str[test_len]);

	string_append_ascii(&test, "Adding another line !\n");
	test_len = (int)strlen(test.str);
	printf(test.str);
	printf("New Length = %d. test[%d] = %c\n", test_len, test_len, test.str[test_len]);

	system("pause");

	// Crash !!

	test.length = 40;
	string_append_ascii(&test, "This won't work.\n");

	return 0;
}