#ifndef COMPILER_INCLUDED
#define COMPILER_INCLUDED

#include <stdio.h>

#include "core_types.h"
#include "vector.h"
#include "ascii_string.h"

// ----------- GENERAL SYMBOLS -------------
// Symbol declarations that are shared across the compiler code.

// Enumerated operator types, in no particular order except for NONE = 0.
enum OPERATOR_TYPE
{
	// None operator, representing an absence of operator.
	OPERATOR_NONE = 0,

	// Mathematical operations, acting over left and right numerical operands.
	OPERATOR_ADD,
	OPERATOR_SUB,
	OPERATOR_MULT,
	OPERATOR_POINTER = OPERATOR_MULT,
	OPERATOR_DIV,
	OPERATOR_MOD,
	OPERATOR_POW,


	OPERATOR_TYPE_COUNT,
};

// Returns the operator type enumeration value associated with the string / longest valid beginning sub-string.
// Also return the length of the string so the user can know how many characters from the input string were "valid".
enum OPERATOR_TYPE op_get_from_string(const char* op_str, int* out_strlen);

// Returns the null-terminated ASCII string associated with the operator type.
// TODO: Consider adding a "Display-friendly" aswell, to differentiate between identifying the operator on a display and identifying it from source code.
const char* op_get_string(enum OPERATOR_TYPE op_val);

// Returns whether operator A has higher priority then operator B.
int op_is_higher_priority(enum OPERATOR_TYPE op_A, enum OPERATOR_TYPE op_B);

struct file_position
{
	int line;
	int col;
	const char* filename;
};

enum
{
	TOKEN_TYPE_IDENTIFIER,
	TOKEN_TYPE_KEYWORD,
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_SYMBOL,
	TOKEN_TYPE_NUMBER,
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_COMMENT,
	TOKEN_TYPE_NEWLINE,
};

enum
{
	TOKEN_FLAG_MULTILINE = 1 << 0,
};

// Value associated to a token, can be one of many types. See the token structure.
union token_value
{
	struct string_ascii strval;
	void* any;
	ui64 llnum;
	ui32 inum;
	enum OPERATOR_TYPE opval;
	enum KEYWORD keywordval;
	char cval;
};

// Result of lexical analysis, associating a type, a value and secondary properties.
struct token
{
	union token_value value;
	struct file_position position;

	// Pointer to start of closest opening parenthesis or brackets.
	const char* enclosure;

	// Type of token, corresponds to token_type enumeration.
	int type;

	// Various bit flags whose exact meaning can depend on token type.
	int flags;

	// Is at least one white space present between this token and the next one ?
	byte whitespace_present;
};

// Enumerated keyword values supported by the compiler.
enum KEYWORD
{
	KEYWORD_NONE = 0,

	KEYWORD_TYPES_START,
	KEYWORD_TYPE_VOID = KEYWORD_TYPES_START,
	KEYWORD_TYPE_CHAR,
	KEYWORD_TYPE_SHORT,
	KEYWORD_TYPE_INT,
	KEYWORD_TYPE_LONG,
	KEYWORD_TYPE_FLOAT,
	KEYWORD_TYPE_DOUBLE,
	KEYWORD_TYPES_END = KEYWORD_TYPE_DOUBLE,

	KEYWORD_IDENTIFIER_MODIFIERS_START,
	KEYWORD_CONST = KEYWORD_IDENTIFIER_MODIFIERS_START,
	KEYWORD_UNSIGNED,
	KEYWORD_SIGNED,
	KEYWORD_VOLATILE,
	KEYWORD_STATIC,
	KEYWORD_EXTERN,
	KEYWORD_IDENTIFIER_MODIFIERS_END = KEYWORD_EXTERN,

	KEYWORD_SPECIAL_OPERATORS_START,
	KEYWORD_SIZEOF,
	KEYWORD_SPECIAL_OPERATORS_END = KEYWORD_SIZEOF,

	KEYWORD_SCOPE_MODIFIERS_START,
	KEYWORD_IF = KEYWORD_SCOPE_MODIFIERS_START,
	KEYWORD_ELSE,
	KEYWORD_DO,
	KEYWORD_WHILE,
	KEYWORD_FOR,
	KEYWORD_SCOPE_MODIFIERS_END = KEYWORD_FOR,

	KEYWORD_TYPE_DECLARATIONS_START,
	KEYWORD_TYPEDEF = KEYWORD_TYPE_DECLARATIONS_START,
	KEYWORD_STRUCT,
	KEYWORD_ENUM,
	KEYWORD_UNION,
	KEYWORD_TYPE_DECLARATIONS_END = KEYWORD_UNION,

	KEYWORD_FLOW_CONTROLS_START,
	KEYWORD_RETURN = KEYWORD_FLOW_CONTROLS_START,
	KEYWORD_GOTO,
	KEYWORD_SWITCH,
	KEYWORD_CASE,
	KEYWORD_BREAK,
	KEYWORD_CONTINUE,
	KEYWORD_FLOW_CONTROLS_END = KEYWORD_CONTINUE,

	KEYWORD_COUNT,
};

// Returns the keyword enumeration value of the passed in keyword string. Returns -1 if keyword wasn't found.
enum KEYWORD keyword_get_from_string(const char* keyword);

// Returns the null-terminated ASCII string associated with the passed keyword. Asserts if not found, as every keyword should be associated with a string !
const char* keyword_get_string(enum KEYWORD kwd);

inline int keyword_is_variable_modifier(enum KEYWORD kwd)
{
	return kwd >= KEYWORD_IDENTIFIER_MODIFIERS_START && kwd <= KEYWORD_IDENTIFIER_MODIFIERS_END;
}

inline int keyword_is_primitive_type(enum KEYWORD kwd)
{
	return kwd >= KEYWORD_TYPES_START && kwd <= KEYWORD_TYPES_END;
}

inline int keyword_is_datatype(enum KEYWORD kwd)
{
	return keyword_is_primitive_type(kwd)
		|| kwd == KEYWORD_STRUCT || kwd == KEYWORD_UNION || kwd == KEYWORD_ENUM; // struct, union and enum can also be used as normal types beyond declaration.
}

// Possible values for the datatype flags bitmask.
enum
{
	DATATYPE_IS_SIGNED =			1 << 0,
	DATATYPE_IS_STATIC =			1 << 1,
	DATATYPE_IS_CONST =				1 << 2,
	DATATYPE_IS_POINTER =			1 << 3,
	DATATYPE_IS_ARRAY =				1 << 4,
	DATATYPE_IS_EXTERN =			1 << 5,
	DATATYPE_IS_ANONYMOUS_STRUCT =	1 << 6,
	DATATYPE_IS_LITERAL =			1 << 7,
	DATATYPE_IS_VOLATILE =			1 << 8,
};

// "Native" data types supported by the compiler.
enum data_type
{
	DATA_TYPE_UNKNOWN,
	DATA_TYPE_VOID,
	DATA_TYPE_CHAR,
	DATA_TYPE_SHORT,
	DATA_TYPE_INT,
	DATA_TYPE_LONG,
	DATA_TYPE_FLOAT,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_STRUCT,
	DATA_TYPE_UNION,
};

size_t data_type_primitive_get_size(int primitive_datatype);

inline int keyword_to_data_type(enum KEYWORD keyword)
{
	assert(keyword_is_datatype(keyword));
	switch (keyword)
	{
	case KEYWORD_TYPE_VOID:
		return DATA_TYPE_VOID;
	case KEYWORD_TYPE_CHAR:
		return DATA_TYPE_CHAR;
	case KEYWORD_TYPE_SHORT:
		return DATA_TYPE_SHORT;
	case KEYWORD_TYPE_INT:
		return DATA_TYPE_INT;
	case KEYWORD_TYPE_LONG:
		return DATA_TYPE_LONG;
	case KEYWORD_TYPE_FLOAT:
		return DATA_TYPE_FLOAT;
	case KEYWORD_TYPE_DOUBLE:
		return DATA_TYPE_DOUBLE;
	case KEYWORD_STRUCT:
		return DATA_TYPE_STRUCT;
	case KEYWORD_UNION:
		return DATA_TYPE_UNION;
	}

	return DATA_TYPE_UNKNOWN;
}

#define DATATYPE_MAX_STRING_LEN (32)

// TODO: Use a previously-determined platform-agnostic "Architecture size" define.
#if _WIN64
#define DATATYPE_POINTER_SIZE (8)
#else
#define DATATYPE_POINTER_SIZE (4)
#endif

// Represents a type known by the compiler.
struct datatype
{
	// Flags bitmask value.
	int flags;

	// Data Type enumeration value.
	enum data_type type;

	// Zero-terminated ASCII name of a user-declared type.
	char type_string[DATATYPE_MAX_STRING_LEN];

	// Size in bytes of this type.
	size_t size;

	// Depth of dereferencing possible with this type.
	int pointer_depth;

	// If this is a complex / structured type, this links to the node used to declare it.
	// The node will either be of type STRUCT or UNION.
	struct parsing_node* declaration_node;

};

// Enumerated types of "Compiled symbols", usable symbols within statements / expressions declared within a scope.
enum COMPILED_SYMBOL_TYPE
{
	SYMBOL_TYPE_VAR, // Symbol references a specific variable.
	SYMBOL_TYPE_FUNC, // Symbol references a function, either just a declaration or a full definition.
	SYMBOL_TYPE_STRUCT, // Symbol references a structured type.
	SYMBOL_TYPE_TYPE, // Symbol references a user-declared type.
};

// Defines a symbol in the sense of a C compilation / linking symbol, linking a name to a broad symbol type,
// and type-dependent value properties. Symbols are aggregated together in Scopes which define when they are usable in the program.
struct compiled_symbol
{
	enum COMPILED_SYMBOL_TYPE symbol_type;
	struct string_ascii symbol_name;

	struct parsing_node* node; // Node associated with this symbol. Expected node type and exact meaning depends on symbol type.
};

enum
{
	SCOPE_FLAG_IS_STRUCT = 1 << 0, // Scope is a structured type scope, which cannot read from parent scopes.
};

// A scope defines a collection of accessible symbols from some point in the program, and a parent scope.
// By default, accessible symbols are those that are part of the current scope or an "ancestor" of current scope.
struct scope
{
	// Scope Flags enum values bitmask.
	int flags;

	// Parent scope, if any (must be defined for all scopes other than the global scope).
	struct scope* parent;

	// Vector of compiled symbols within this scope.
	struct vector symbols;
};

// ------- COMPILER --------------

struct compile_process_input_file
{
	FILE* file;
	const char* file_abs_path;
};

// Return values for the compile_file function.
enum
{
	COMPILER_FILE_COMPILED_OK, // Compilation successful.
	COMPILER_FATAL_ERROR, // Fatal error in the compiler process itself, not due to user input.
	COMPILER_FAILED_WITH_ERRORS, // Compilation error of any kind (check this or above for "normal" errors related to user input).

	COMPILER_LEXER_ERROR, // Failed at the Lexical Analysis stage.
	COMPILER_PARSER_ERROR, // Failed at the Parsing stage.
};

// Ongoing compilation process.
struct compile_process
{
	// Flags in regards to how this file should be compiled.
	int flags;

	// Current position of the compilation process.
	struct file_position position;

	// Input file structure containing the FILE handle and its absolute path.
	struct compile_process_input_file input_file;

	FILE* output_file;

	// Intermediate values

	// Contains all tokens that should be taken into account in the parsing stage.
	struct vector token_vec;

	// Contains all nodes from the parsing stage.
	struct vector node_vec;

	// Contains POINTERS to all independent root nodes from the parsing stage.
	struct vector node_tree_vec;

	// Vector of all scopes in order of creation, with index 0 being the global scope.
	struct vector scopes;

	// Output Status

	// COMPILER_ERROR enum value, indicating an error in the compiler process itself or which stage of the process failed.
	int compiler_error;

	// If compiler_error indicates an error in a specific stage, this contains an error code relevant to that stage.
	int stage_error;
};


struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);
void compile_process_destroy(struct compile_process* process);

// Emits an error message in the standard error stream and populates the error codes in the compiler process structure.
// The compilation process should stop and return as soon as possible after this.
void compiler_error_v(struct compile_process* compiler, int compiler_error_code, int stage_error,
	const char* msg, va_list args);

// Emits an error message in the standard error stream and populates the error codes in the compiler process structure.
// The compilation process should stop and return as soon as possible after this.
void compiler_error(struct compile_process* compiler, int compiler_error_code, int stage_error,
	const char* msg, ...);

// Returns whether the compiler is in an error state. When true the compiler should seek to exit as fast as possible.
inline int compiler_has_error(struct compile_process* compiler) { return compiler->compiler_error > 0; }

// Creates a new scope within the compiler's scope collection with the passed in parent scope, and returns a pointer to it.
// If parent is NULL, will use Global Scope.
struct scope* compiler_push_scope(struct compile_process* compiler, struct scope* parent);

struct scope* scope_get_global(struct compile_process* compiler);

// Adds a new symbol (by copy) to the scope. Returns whether the symbol was successfuly pushed.
// If not, the problem usually is that a symbol with the same name already exists in this scope.
int scope_push_symbol(struct scope* scope, struct compiled_symbol symbol);

// Searches for and returns a symbol by name from the specified scope or one of its ancestors if applicable.
// NULL return means the symbol is unknown or unaccessible from the specified scope.
struct compiled_symbol* scope_get_symbol(struct scope* scope, const char* symbol_name);

// Searches for and returns a symbol by name from the specified scope ONLY.
// NULL return means the symbol does not exist in the scope (but could exist in a parent scope).
struct compiled_symbol* scope_get_symbol_local(struct scope* scope, const char* symbol_name);

// Main compilation function & stages.

// Start compilation for a specific file. Returns one of the COMPILER_* enumeration values.
// If failure happened inside one of the stages, will return a general enum value indicating which stage,
// and out_stage_error will be populated with the stage-specific error code.
int compile_file(const char* filename, const char* out_filename, int flags, int* out_stage_error);

int lex(struct lex_process* lexer);
int parse(struct compile_process* compiler);

// ----------- LEXER -------------

struct lex_process;

typedef char (*LEX_PROCESS_NEXT_CHAR_FUNC)(struct lex_process* process);
typedef char (*LEX_PROCESS_PEEK_CHAR_FUNC)(struct lex_process* process);
typedef void (*LEX_PROCESS_PUSH_CHAR_FUNC)(struct lex_process* process, char c);

struct lex_process_functions
{
	LEX_PROCESS_NEXT_CHAR_FUNC next_char;
	LEX_PROCESS_PEEK_CHAR_FUNC peek_char;
	LEX_PROCESS_PUSH_CHAR_FUNC push_char;
};

// Lexer stage error codes.
enum
{
	LEXER_ALL_OK,
	LEXER_FATAL_ERROR,
	LEXER_INPUT_ERROR,
};

struct lex_process
{
	struct file_position position;
	struct vector token_vec;
	struct compile_process* compiler;

	// How many brackets / parenthesis are open at the current position.
	int current_expression_count;
	byte* parentheses_buffer;

	struct lex_process_functions functions;

	// Private data the user of the lexer can transfer to the process.
	void* private;
};

struct lex_process* lex_process_create(struct compile_process* compiler, void* private_data);
void lex_process_destroy(struct lex_process* lexer);

// --------- PARSER -----------

// Parser stage error codes.
enum
{
	PARSER_ALL_OK,
	PARSER_FATAL_ERROR, // Parser internal error due to incomplete / faulty parser code, or fatal platform function failure.
	PARSER_NO_TOKENS, // Parser could not find parseable tokens in the compiler process.
	PARSER_GENERAL_ERROR, // General error usually stemming from faulty input.
};

// Parser node types.
enum
{
	NODE_TYPE_EXPRESSION,
	NODE_TYPE_NUMBER,
	NODE_TYPE_IDENTIFIER,
	NODE_TYPE_STRING,
	NODE_TYPE_VARIABLE,
	NODE_TYPE_VARIABLE_LIST,
	NODE_TYPE_FUNCTION,
	NODE_TYPE_BODY,
	NODE_TYPE_STATEMENT_RETURN,
	NODE_TYPE_STATEMENT_IF,
	NODE_TYPE_STATEMENT_ELSE,
	NODE_TYPE_STATEMENT_WHILE,
	NODE_TYPE_STATEMENT_DO_WHILE,
	NODE_TYPE_STATEMENT_FOR,
	NODE_TYPE_STATEMENT_BREAK,
	NODE_TYPE_STATEMENT_CONTINUE,
	NODE_TYPE_STATEMENT_SWITCH,
	NODE_TYPE_STATEMENT_CASE,
	NODE_TYPE_STATEMENT_DEFAULT,
	NODE_TYPE_STATEMENT_GOTO,


	NODE_TYPE_STATEMENT_UNARY,
	NODE_TYPE_STATEMENT_TERNARY,
	NODE_TYPE_STATEMENT_LABEL,
	NODE_TYPE_STATEMENT_STRUCT,
	NODE_TYPE_STATEMENT_UNION,
	NODE_TYPE_STATEMENT_BRACKETS,
	NODE_TYPE_STATEMENT_CAST,
	NODE_TYPE_STATEMENT_BLANK,
};

struct parsing_node
{
	int type;
	int flags;

	struct file_position position;

	struct
	{
		// Pointer to body node.
		struct parsing_node* owner;

		// Pointer to function node this node is in.
		struct parsing_node* function;
	} context;

	union
	{
		struct parsing_node_exp_value
		{
			struct parsing_node* left;
			struct parsing_node* right;

			enum OPERATOR_TYPE op;
			int parenthesis_level;
		} exp;

		char cval;
		struct string_ascii sval;
		ui32 inum;
		ui64 llnum;	
	} value;
};

inline int node_is_expressionable(struct parsing_node* node)
{
	return node != NULL &&
	(
			node->type == NODE_TYPE_EXPRESSION
		||	node->type == NODE_TYPE_STATEMENT_UNARY
		||	node->type == NODE_TYPE_NUMBER
		||	node->type == NODE_TYPE_STRING
		||	node->type == NODE_TYPE_IDENTIFIER
	);
}

// Recursively prints the contents and of its children if any into the standard output.
void print_node(struct parsing_node* node, int indentation);

// Returns a pointer to the last node in the passed in node vector, or NULL if the vector is empty.
struct parsing_node* node_peek(struct vector* node_vec);

// Pops the last node from the node vector, and of the root node vector if it is the same node.
// Populates out_popped if something was popped.
// Used to construct more complex nodes from constituent nodes.
int node_pop(struct vector* node_vec, struct vector* root_node_vec, struct parsing_node* out_popped);

#endif // COMPILER_INCLUDED