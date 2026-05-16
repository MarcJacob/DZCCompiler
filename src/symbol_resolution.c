#include "compiler.h"

// Implementation file for the Symbols & Scopes Resolution post-process step of the Parser.

// Parses a new variable symbol into the specified owner scope.
static int parse_symbol_variable(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* variable_node)
{
	assert(variable_node != NULL && variable_node->type == NODE_TYPE_VARIABLE);
}

static int parse_symbol_function(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* function_node)
{
	assert(function_node != NULL && function_node->type == NODE_TYPE_FUNCTION);
}

static int parse_symbol_structure(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* structure_node)
{
	assert(structure_node != NULL && structure_node->type == NODE_TYPE_STATEMENT_STRUCT);
}

int generate_symbols(struct compile_process* compiler)
{
	// Read through the generated node trees in order to form the top-level global scope symbols.
	// Function and Structure symbols will start the creation of scope sub-trees containing their own symbols.

	printf("Beginning symbol generation...\n\n");

	for (int tree_index = 0; tree_index < compiler->node_tree_vec.size; tree_index++)
	{
		struct parsing_node** root = vector_get_ptr(compiler->node_tree_vec, tree_index);
	}

	printf("\nSymbol generation complete.\n\n");

	return 1;
}
