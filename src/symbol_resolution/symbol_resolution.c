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

// Returns whether the two passed in function symbols are *equivalent*, meaning they share a return type, name, and parameter types.
static int function_symbols_equivalent(struct compiled_symbol* symbol_A, struct compiled_symbol* symbol_B)
{
	assert(symbol_A != NULL && symbol_B != NULL);

	// Compare symbol names.
	if (strcmp(symbol_A->symbol_name.str, symbol_B->symbol_name.str) != 0)
	{
		return 0;
	}

	// Compare types.
	if (symbol_A->symbol_type != symbol_B->symbol_type)
	{
		return 0;
	}

	struct parsing_node* func_A = symbol_A->node;
	struct parsing_node* func_B = symbol_B->node;
	assert(func_A != NULL && func_B != NULL);

	// Compare return types.
	// TODO: Consider adding a broad datatype comparison function that checks the contents of the datatype structures instead of their names (in case of typedefs).
	if (strcmp(func_A->value.func.return_type.type_string, func_B->value.func.return_type.type_string) != 0)
	{
		// Return types do not match = symbols are not equivalent.
		return 0;
	}

	// Compare parameter count and types.
	if (func_A->value.func.param_var_nodes.size != func_B->value.func.param_var_nodes.size)
	{
		// Param counts do not match = symbols are not equivalent.
		return 0;
	}

	for (int param_index = 0; param_index < func_A->value.func.param_var_nodes.size; param_index++)
	{
		struct parsing_node* param_A = vector_get_val(func_A->value.func.param_var_nodes, param_index, struct parsing_node*);
		struct parsing_node* param_B = vector_get_val(func_B->value.func.param_var_nodes, param_index, struct parsing_node*);

		struct datatype* param_type_A = &param_A->value.var.type;
		struct datatype* param_type_B = &param_B->value.var.type;

		// If any parameter of a given index does not share the same type, then the symbols are not equivalent.
		if (strcmp(param_type_A->type_string, param_type_B->type_string) != 0)
		{
			return 0;
		}
	}

	return 1;
}

static int parse_symbol_function(struct compile_process* compiler, struct scope* owner_scope, struct parsing_node* function_node)
{
	assert(function_node != NULL && function_node->type == NODE_TYPE_FUNCTION);

	// Create new symbol for the function and add it to the owner scope.
	struct compiled_symbol func_symbol = { 0 };
	func_symbol.node = function_node;
	// Allocate string for the symbol's name. It will have to be freed in case of error.
	func_symbol.symbol_name = string_create_ascii(function_node->value.func.name);
	func_symbol.symbol_type = SYMBOL_TYPE_FUNC;

	if (!scope_push_symbol(owner_scope, func_symbol))
	{
		struct compiled_symbol* conflicting = scope_get_symbol_local(owner_scope, func_symbol.symbol_name.str);
		if (conflicting == NULL)
		{
			// Error when pushing symbol, but no conflicting symbol in the local scope = unhandled error. Abandon ship.
			compiler_error(compiler, COMPILER_SYMBOL_RESOLVER_ERROR, SYMBOL_RESOLVER_FATAL_ERROR, "Unkown error adding function symbol '%s' to scope.", func_symbol.symbol_name.str);
			string_free_ascii(&func_symbol.symbol_name);
			return 0;
		}

		// Failed to push symbol, meaning another symbol with the same name exists in the same scope.
		// Only emit an error if this is a *redefinition* or a *name conflict* (because of difference in return type or parameters).
		if (function_symbols_equivalent(&func_symbol, conflicting))
		{
			// Return success without pushing anything to the scope, effectively ignoring the new symbol as a perfect duplicate of an already-declared symbol.
			string_free_ascii(&func_symbol.symbol_name);
			return 1;
		}
		else
		{
			// Non-equivalent function symbols with the same scope and name = redefinition error.
			compiler_error(compiler, COMPILER_SYMBOL_RESOLVER_ERROR, SYMBOL_RESOLVER_GENERAL_ERROR, "Symbol redefinition for function '%s'.", func_symbol.symbol_name.str);
			string_free_ascii(&func_symbol.symbol_name);
			return 0;
		}
	}
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
	printf("\nParsed scopes & symbols:\n");
	compiler_print_scope_tree(compiler);
	printf("\n");

	return 1;
}
