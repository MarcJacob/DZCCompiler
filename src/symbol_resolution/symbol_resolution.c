#include "compiler.h"

// Implementation file for the Symbols & Scopes Resolution post-process step of the Parser.

// Parses a new variable symbol into the specified owner scope.
static int parse_symbol_variable(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* variable_node)
{
	assert(variable_node != NULL && variable_node->type == NODE_TYPE_VARIABLE);

	// Create new symbol for the variable and add it to the owner scope.
	struct compiled_symbol var_symbol = {0};
	var_symbol.node = variable_node;
	var_symbol.symbol_name = string_create_ascii(variable_node->value.var.name);
	var_symbol.symbol_type = SYMBOL_TYPE_VAR;

	scope_push_symbol(owner_scope, var_symbol);
	return 1;
}

static int parse_symbol_function(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* function_node)
{
	assert(function_node != NULL && function_node->type == NODE_TYPE_FUNCTION);

	// Create new symbol for the function and add it to the owner scope.
	struct compiled_symbol func_symbol = { 0 };
	func_symbol.node = function_node;
	func_symbol.symbol_name = string_create_ascii(function_node->value.func.name);
	func_symbol.symbol_type = SYMBOL_TYPE_FUNC;

	scope_push_symbol(owner_scope, func_symbol);
	return 1;
}

static int parse_symbol_structure(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* structure_node)
{
	assert(structure_node != NULL && structure_node->type == NODE_TYPE_STATEMENT_STRUCT);
}

int resolve_symbols(struct compile_process* compiler)
{
	// Read through the generated node trees in order to form the top-level global scope symbols.
	// Function and Structure symbols will start the creation of scope sub-trees containing their own symbols.

	printf("Beginning symbol generation...\n");

	for (ui16 tree_index = 0; tree_index < compiler->node_tree_vec.size; tree_index++)
	{
		struct parsing_node* root = *(struct parsing_node**)vector_get_ptr(compiler->node_tree_vec, tree_index);
		switch (root->type)
		{
		case NODE_TYPE_VARIABLE:
			parse_symbol_variable(compiler, compiler_get_global_scope(compiler), root);
			break;
		case NODE_TYPE_FUNCTION:
			parse_symbol_function(compiler, compiler_get_global_scope(compiler), root);
			break;
		}
	}

	printf("Symbol generation complete.\n");

	// Report on parsed scopes / symbols.
	printf("\n\nParsed scopes & symbols:\n\n");
	compiler_print_scope_tree(compiler);
	printf("\n");

	return 1;
}
