#include "compiler.h"

#include <stdarg.h>

// Compiler process function implementations.

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
	if (parse(compiler) != PARSER_ALL_OK)
	{
		COMPILER_EXIT_FAILURE();
	}

	// Perform code generation
	
	compile_process_destroy(compiler);
	COMPILER_EXIT_SUCCESS();
}
#undef COMPILER_EXIT_FAILURE

// General compiler symbol implementations.

// Operator system.

// Associates an operator type value to a null-terminated ASCII string, usually one or two characters long.
struct op_string_pairing
{
	enum OPERATOR_TYPE op_val;
	const char* op_str;
};

static struct op_string_pairing OP_STRING_PAIRING_TABLE[] =
{
	{ OPERATOR_ADD,		"+" },
	{ OPERATOR_SUB,		"-" },
	{ OPERATOR_MULT,	"*" },
	{ OPERATOR_DIV,		"/" },
	{ OPERATOR_MOD,		"%" },
	{ OPERATOR_POW,		"^>" },
};

enum OPERATOR_TYPE op_get_from_string(const char* op_str, int* out_strlen)
{
	assert(op_str != NULL);
	assert(out_strlen != NULL);

	// Go through the table linearly and return the first match by a simple string compare.
	static int TABLE_SIZE = sizeof(OP_STRING_PAIRING_TABLE) / sizeof(struct op_string_pairing);

	const size_t op_str_len = strlen(op_str);

	enum OPERATOR_TYPE op = OPERATOR_NONE;
	*out_strlen = 0;
	for (int pairing_index = 0; pairing_index < TABLE_SIZE; pairing_index++)
	{
		// Perform a simple character-by-character comparison returning the number of common characters up to the length of the input string
		// or the pairing string.

		const char* pairing_str = OP_STRING_PAIRING_TABLE[pairing_index].op_str;
		const size_t pairing_str_len = strlen(pairing_str);

		// Count number of common characters.
		int ci = 0;
		for (; ci < op_str_len && ci < pairing_str_len 
								&& op_str[ci] == pairing_str[ci];
		ci++)
		{}

		// ci == pairing_str_len means the entirety of the operator was contained within the input string.
		// If there are more common characters than whatever was found before, use that pairing.
		if (ci == pairing_str_len && ci > *out_strlen)
		{
			op = OP_STRING_PAIRING_TABLE[pairing_index].op_val;
			*out_strlen = ci;
		}
	}

	return op;
}

const char* op_get_string(enum OPERATOR_TYPE op_val)
{
	assert(op_val > 0 && op_val < OPERATOR_TYPE_COUNT);
	if (op_val == OPERATOR_NONE) return "<NO OP>";

	static int TABLE_SIZE = sizeof(OP_STRING_PAIRING_TABLE) / sizeof(struct op_string_pairing);

	int op_index = 0;
	for (; op_index < TABLE_SIZE; op_index++)
	{
		if (op_val == OP_STRING_PAIRING_TABLE[op_index].op_val)
		{
			break;
		}
	}

	// Assert that a valid value was found and return the pairing string.
	assert(op_index < TABLE_SIZE);
	return OP_STRING_PAIRING_TABLE[op_index].op_str;
}

// NONE-terminated group of operators.
typedef enum OPERATOR_TYPE operator_group[32];

// Contains groups of operators in order of descending priority. Operators that are part of the same group are considered to be the same priority.
static operator_group OP_PRIORITY_TABLE[] =
{
	{ OPERATOR_POW, },
	{ OPERATOR_MULT, OPERATOR_DIV, OPERATOR_MOD, },
	{ OPERATOR_ADD, OPERATOR_SUB, },
};

int op_is_higher_priority(enum OPERATOR_TYPE op_A, enum OPERATOR_TYPE op_B)
{
	// Look through the priority table and find whichever group both operators are part of.
	// If either operator is not part of any group or more than one group, assert.

	static int GROUP_COUNT = sizeof(OP_PRIORITY_TABLE) / sizeof(operator_group);

	int op_A_group = -1;
	int op_B_group = -1;

	for (int group_index = 0; group_index < GROUP_COUNT; group_index++)
	{
		for (int op_index = 0; OP_PRIORITY_TABLE[group_index][op_index] != OPERATOR_NONE; op_index++)
		{
			enum OPERATOR_TYPE op = OP_PRIORITY_TABLE[group_index][op_index];
			if (op_A == op)
			{
				// Check that operator A isn't defined twice in the table.
				assert(op_A_group == -1);
				op_A_group = group_index;
			}
			if (op_B == op)
			{
				// Check that operator B isn't defined twice in the table.
				assert(op_B_group == -1);
				op_B_group = group_index;
			}

			if (op_A_group >= 0 && op_B_group >= 0) break;
		}

		if (op_A_group >= 0 && op_B_group >= 0) break;
	}

	// If either of those fire, we're missing an operator in the table.
	assert(op_A_group >= 0);
	assert(op_B_group >= 0);

	// Return whether operator A is in a "higher" group than B (meaning strictly lower index).
	return op_A_group < op_B_group;
}

// Pairs together a KEYWORD value and its string representation.
struct keyword_string_pairing
{
	enum KEYWORD kwd;
	const char* str;
};

// Global lookup table to determine whether a string is a keyword, and its index.
// Must be aligned to 
static const struct keyword_string_pairing KEYWORD_STRING_PAIRING_TABLE[] =
{
	// Primitive types
	{ KEYWORD_TYPE_CHAR, "char" },
	{ KEYWORD_TYPE_SHORT, "short" },
	{ KEYWORD_TYPE_INT, "int"},
	{ KEYWORD_TYPE_LONG, "long"},
	{ KEYWORD_TYPE_FLOAT, "float"},
	{ KEYWORD_TYPE_DOUBLE, "double"},

	// Identifier modifiers
	{ KEYWORD_CONST, "const" },
	{ KEYWORD_UNSIGNED, "unsigned" },
	{ KEYWORD_VOLATILE, "volatile" },
	{ KEYWORD_STATIC, "static" },
	{ KEYWORD_EXTERN, "extern" },

	// Special operators
	{ KEYWORD_SIZEOF, "sizeof" },

	// Scope modifiers
	{ KEYWORD_IF, "if" },
	{ KEYWORD_ELSE, "else" },
	{ KEYWORD_DO, "do" },
	{ KEYWORD_WHILE, "while" },
	{ KEYWORD_FOR, "for" },

	// Type declarations
	{ KEYWORD_TYPEDEF, "typedef" },
	{ KEYWORD_STRUCT, "struct" },
	{ KEYWORD_ENUM, "enum" },
	{ KEYWORD_UNION, "union" },

	// Flow controls

	{ KEYWORD_RETURN, "return" },
	{ KEYWORD_GOTO, "goto" },
	{ KEYWORD_SWITCH, "switch" },
	{ KEYWORD_CASE, "case" },
	{ KEYWORD_BREAK, "break" },
	{ KEYWORD_CONTINUE, "continue" },
};


enum KEYWORD keyword_get_from_string(const char* str)
{
	const int keywords_table_size = sizeof(KEYWORD_STRING_PAIRING_TABLE) / sizeof(struct keyword_string_pairing);
	for (int pairing_index = 0; pairing_index < keywords_table_size; pairing_index++)
	{
		if (strcmp(str, KEYWORD_STRING_PAIRING_TABLE[pairing_index].str) == 0)
		{
			return KEYWORD_STRING_PAIRING_TABLE[pairing_index].kwd;
		}
	}

	return KEYWORD_NONE;
}

const char* keyword_get_string(enum KEYWORD kwd)
{
	const int keywords_table_size = sizeof(KEYWORD_STRING_PAIRING_TABLE) / sizeof(char*);
	int pairing_index = 0;
	for (; pairing_index < keywords_table_size; pairing_index++)
	{
		if (kwd == KEYWORD_STRING_PAIRING_TABLE[pairing_index].kwd)
		{
			break;
		}
	}

	assert(pairing_index != keywords_table_size);
	return KEYWORD_STRING_PAIRING_TABLE[pairing_index].str;
}
