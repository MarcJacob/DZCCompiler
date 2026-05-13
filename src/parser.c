#include "compiler.h"

struct parser_token_list
{
	// A parser token wraps a token with a pointer to a "next" token, collectively forming a chain of parsable tokens
	// to be travelled up and down recursively by the tree-like structure of the parsing process.
	struct parser_token
	{
		struct token* token;
		struct parser_token* next;
	} *head;
};

// Builds a linked list of pointers to tokens the parser knows how to parse.
static struct parser_token_list build_parser_token_list(struct compile_process* compiler)
{
	struct parser_token_list list = { 0 };
	struct parser_token* item = NULL;

	// Go through each token and check their type. Only certain types are supported.
	for (int token_index = 0; token_index < compiler->token_vec.size; token_index++)
	{
		struct token* token = vector_get_ptr(compiler->token_vec, token_index);
		struct parser_token* new_item = NULL;
		switch (token->type)
		{
		case TOKEN_TYPE_NUMBER:
		case TOKEN_TYPE_IDENTIFIER:
		case TOKEN_TYPE_STRING:
		case TOKEN_TYPE_OPERATOR:
		case TOKEN_TYPE_SYMBOL:
		case TOKEN_TYPE_KEYWORD:
		case TOKEN_TYPE_NEWLINE: // Temporary, until we introduce end of statement tokens.
			new_item = calloc(1, sizeof(struct parser_token));
			assert(new_item != NULL);

			new_item->token = token;

			if (item) item->next = new_item;
			else list.head = new_item;

			item = new_item;
		default:
			break;
		}
	}

	return list;
}

// Frees a previously-build parser token list.
static void free_parser_token_list(struct parser_token_list* list)
{
	if (!list->head) return; // No head = no content to free.

	// Free the entire list by starting at the head (and nulling it out), going to the next item and
	// freeing the previous item. This is a sort of "forward freeing" approach.
	struct parser_token* item = list->head;
	list->head = NULL;
	while (item->next)
	{
		struct parser_token* previous = item;
		item = item->next;
		free(previous);
	}

	// Free last item.
	free(item);
}

// "Context" structure for parsing a node tree.
// The structure does not actually own any of the pointers, it is purely indicative.
struct tree_parsing_context
{
	struct compile_process* compiler;
	struct parser_token* root_token;

	// How many levels "deep" are we in parenthesis symbols ?
	// Assigned to any new expression node created in this tree. 
	// The main consequence is that operator priority is ignored between expression nodes at different levels.
	int parenthesis_level;
};

// Copies the passed node into the tree's compiler's node vectors, and frees the node.
// Also populates the node's context.
static void push_node_to_tree(struct tree_parsing_context* tree, struct parsing_node* node)
{
	vector_push(tree->compiler->node_vec, *node);
	struct parsing_node* pushed = vector_back_ptr(&tree->compiler->node_vec);
	vector_push(tree->compiler->node_tree_vec, pushed);
	free(node);

	// TODO: Set node context.
}

// Creates a new node from the passed in token, if the token is of a compatible type.
// Single-token nodes 
static int parse_single_token_node(struct tree_parsing_context* tree, struct parser_token* token)
{
	struct parsing_node* new_node = calloc(1, sizeof(struct parsing_node));
	assert(new_node != NULL);

	switch (token->token->type)
	{
	case TOKEN_TYPE_NUMBER:
		new_node->type = NODE_TYPE_NUMBER;
		new_node->value.llnum = token->token->value.llnum;
		break;
	case TOKEN_TYPE_IDENTIFIER:
		new_node->type = NODE_TYPE_IDENTIFIER;
		new_node->value.sval = token->token->value.strval;
		break;
	case TOKEN_TYPE_STRING:
		new_node->type = NODE_TYPE_STRING;
		new_node->value.sval = token->token->value.strval;
		break;
	default:
		free(new_node);
		return -1;	
	}

	new_node->position = token->token->position;
	push_node_to_tree(tree, new_node);
	return 0;
}

// Creates a new expression node from operands and an operator string.
// This is a "builder" function, it does NOT push the node to any vector or tree.
// Returns the new node, or the operand which ends up being the top-level node because of operator precedence.
struct parsing_node* build_expression_node(struct parsing_node* left_operand, struct parsing_node* right_operand, enum OPERATOR_TYPE op, int parenthesis_level)
{
	assert(left_operand != NULL);
	assert(right_operand != NULL);

	// Build new "original parent" node with the specific input operator and original precedence over the two input operands.
	struct parsing_node* exp_node = calloc(1, sizeof(struct parsing_node));
	assert(exp_node != NULL);

	exp_node->type = NODE_TYPE_EXPRESSION;
	exp_node->value.exp.parenthesis_level = parenthesis_level;
	exp_node->value.exp.op = op;

	exp_node->value.exp.left = left_operand;

	// Handle operator precedence with right operand if that operand is itself an expression and is located at the same parenthesis level,
	// and has an operator of LOWER priority than the this expression's.
	if (right_operand->type == NODE_TYPE_EXPRESSION && parenthesis_level == right_operand->value.exp.parenthesis_level
		&& op_is_higher_priority(op, right_operand->value.exp.op))
	{
		// If the right operand is of a LOWER priority than this one, this node should become that operand expression's LEFT operand,
		// while the previous expression there should become this node's right operand.
		// IE, if we have 1 / (2 * 3) + 4, the division will initially be the top level expression, but needs to become the addition's LEFT operand,
		// and the (2 * 3) needs to become the division's right operand instead.a

		struct parsing_node* previous_left = right_operand->value.exp.left;
		right_operand->value.exp.left = exp_node;
		exp_node->value.exp.right = previous_left;
		
		// Return the actual top-level node, which ends up being the input right operand.
		return right_operand;
	}

	// Otherwise just assign it as the right operand of this expression with no further worry.
	exp_node->value.exp.right = right_operand;
	return exp_node;
}

static int parse_expressionable(struct tree_parsing_context* tree, struct parser_token** start_token);

static int parse_expressionable_for_op(struct tree_parsing_context* tree, struct parser_token** start_token, enum OPERATOR_TYPE op)
{
	return parse_expressionable(tree, start_token);
}

// Parses a full expression node from an operator token.
static int parse_expression(struct tree_parsing_context* tree, struct parser_token** start_token)
{
	struct parser_token* token = *start_token;
	assert(token != NULL && token->token->type == TOKEN_TYPE_OPERATOR);

	// The left operand is parsed BEFORE this node, so it should be the last item in the compiler's node vec.
	if (!node_is_expressionable(node_peek(&tree->compiler->node_vec)))
	{
		return -1; // Left operand is not expressionable, so it cannot form an expression with the operator.
	}

	// Left operand was found and can be popped off.
	struct parsing_node* left_operand = calloc(1, sizeof(struct parsing_node));
	assert(left_operand != NULL);

	node_pop(&tree->compiler->node_vec, &tree->compiler->node_tree_vec, left_operand);

	// Now parse next node and attempt to use it as right operand.
	// Cache parenthesis level here as the level that will be associated with the final expression node.
	int exp_parenthesis_level = tree->parenthesis_level;
	struct parser_token* right_operand_start_token = token->next;
	const enum OPERATOR_TYPE op = token->token->value.opval;
	if (parse_expressionable_for_op(tree, &right_operand_start_token, op) != 0)
	{
		// Right token was not an expressionable.
		free(left_operand);
		return -1;
	}

	// Extract parsed node from the node vector.
	struct parsing_node* right_operand = calloc(1, sizeof(struct parsing_node));
	assert(right_operand != NULL);
	node_pop(&tree->compiler->node_vec, &tree->compiler->node_tree_vec, right_operand);

	struct parsing_node* exp_node = build_expression_node(left_operand, right_operand, op, exp_parenthesis_level);

	push_node_to_tree(tree, exp_node);

	// Update start_token pointer to point to next token.
	// After parsing the right operand, its start token pointer should be equal to the token coming after.
	*start_token = right_operand_start_token;

	return 0;
}

// Check for parenthesis symbols from the start token and updates it to point to the first non-parenthesis token encountered.
// Adapt tree parenthesis level according to encountered open / closed parenthesis.
static void handle_parenthesis(struct tree_parsing_context* tree, struct parser_token** start_token)
{
	struct parser_token* token = *start_token;
	while (token != NULL && token->token->type == TOKEN_TYPE_SYMBOL)
	{
		// Check for parenthesis open / close and change the tree's current parenthesis level accordingly.
		// Fail if we go into the negative levels.
		if (token->token->value.cval == '(')
		{
			tree->parenthesis_level++;
		}
		else if (token->token->value.cval == ')')
		{
			tree->parenthesis_level--;	
			
			if (tree->parenthesis_level < 0)
			{
				tree->compiler->position = (*start_token)->token->position;
				compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Encountered closing parenthesis with no matching opening.");
				return;
			}
		}
		else
		{
			break;
		}

		token = token->next;
	}

	*start_token = token;
}

// An "expressionable" is a node that can form an expression with an operator and other expressionables.
// This is also where we handle parenthesis level increase or decrease.
static int parse_expressionable(struct tree_parsing_context* tree, struct parser_token** start_token)
{	
	// Cache parenthesis level of this "expressionable scope".
	int parenthesis_level = tree->parenthesis_level;

	// Handle pre-expression parenthesis.
	handle_parenthesis(tree, start_token);
	if (compiler_has_error(tree->compiler))
	{
		return -1;
	}


	struct parser_token* parser_token = *start_token;
	struct parsing_node* node = NULL;
	int parse_res = -1;

	// Keep parsing tokens until we've reached back "above" the entry parenthesis level or a suitable end-of-expression symbol.
	while (tree->parenthesis_level >= parenthesis_level)
	{
		switch (parser_token->token->type)
		{
		case TOKEN_TYPE_NUMBER:
			parse_res = parse_single_token_node(tree, parser_token);
			parser_token = parser_token->next;
			break;
		case TOKEN_TYPE_OPERATOR:
			parse_res = parse_expression(tree, &parser_token);
			break;
		}
		
		if (parse_res != 0) break; // Break out immediately on parsing error.

		// After every successful parse, handle any new parenthesis that may be encountered.
		handle_parenthesis(tree, &parser_token);
		if (compiler_has_error(tree->compiler))
		{
			return -1;
		}

		if (parser_token == NULL || parser_token->token->type == TOKEN_TYPE_NEWLINE) // Break out when reaching something that must end the expression.
		{
			// Check that we at the top parenthesis level.
			if (tree->parenthesis_level > 0)
			{
				parse_res = -1;
				tree->compiler->position = node_peek(&tree->compiler->node_vec)->position;
				compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Reached end of expression with unclosed parenthesis scopes.");
			}
			break;		
		}

	}
	
	if (parse_res == 0)
	{
		// Update start_token pointer to point to next token.
		*start_token = parser_token;
	}
	else // Error while parsing expression. Return error code after restoring tree's starting parenthesis level.
	{
		tree->parenthesis_level = parenthesis_level;
		return parse_res;
	}
	
	return 0;
}

static int parse_identifier(struct tree_parsing_context* tree, struct parser_token** start_token)
{
	// Temp: Advance by one token and return.
	*start_token = (*start_token)->next;
	return 0;
}

// Parse the next handful of tokens into a datatype, a structure describing a datatype we are interested in for declaring a variable, structure,
// union or function.
static struct datatype parse_datatype(struct tree_parsing_context* tree, struct parser_token** token_list)
{
	struct datatype type = {0};
	type.flags |= DATATYPE_IS_SIGNED;

	// Parse prefixed modifiers, the type name itself, then postfixed modifiers.
	struct parser_token* token = *token_list;
	if (token == NULL)
	{
		compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_NO_TOKENS, "No tokens to parse into datatype.");
		return type;
	}


	// Prefixed modifiers
	while (token != NULL && token->token->type == TOKEN_TYPE_KEYWORD
		&& keyword_is_variable_modifier(token->token->value.keywordval))
	{
		// Parse modifier token -> datatype flags.
		switch (token->token->value.keywordval)
		{
		case KEYWORD_SIGNED:
			type.flags |= DATATYPE_IS_SIGNED;
			break;
		case KEYWORD_UNSIGNED:
			type.flags &= ~DATATYPE_IS_SIGNED;
			break;
		case KEYWORD_CONST:
			type.flags |= DATATYPE_IS_CONST;
			break;
		case KEYWORD_STATIC:
			type.flags |= DATATYPE_IS_STATIC;
			break;
		case KEYWORD_EXTERN:
			type.flags |= DATATYPE_IS_EXTERN;
			break;
		case KEYWORD_VOLATILE:
			type.flags |= DATATYPE_IS_VOLATILE;
			break;
		default:
			tree->compiler->position = token->token->position;
			compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_FATAL_ERROR, "Unhandled variable modifier keyword when parsing data type.");
			return type;
		}

		token = token->next;
	}

	if (token == NULL)
	{
		tree->compiler->position = (*token_list)->token->position;
		compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Ran out of tokens parsing data type.");
		return type;
	}

	// Datatype string
	if (token->token->type == TOKEN_TYPE_KEYWORD && keyword_is_datatype(token->token->value.keywordval))
	{
		// Parse datatype type from one of the primitive types.
		type.type = keyword_to_data_type(token->token->value.keywordval);

		if (type.type == DATA_TYPE_STRUCT || type.type == DATA_TYPE_UNION)
		{
			// Handle structured type - an identifier needs to be parsed.
			token = token->next;
			if (token->token->type != TOKEN_TYPE_IDENTIFIER)
			{
				// Next token isn't an identifier, meaning the only possibility is the declaration of an anonymous structured type.
				type.flags |= DATATYPE_IS_ANONYMOUS_STRUCT;

				// Generate random name for anonymous structured type
				if (type.type == DATA_TYPE_STRUCT)
				{
					static size_t ANONYMOUS_STRUCT_COUNTER = 0;
					sprintf_s(type.type_string, DATATYPE_MAX_STRING_LEN, "ANON_STRUCT_%lld", ANONYMOUS_STRUCT_COUNTER++);
				}
				else
				{
					static size_t ANONYMOUS_UNION_COUNTER = 0;
					sprintf_s(type.type_string, DATATYPE_MAX_STRING_LEN, "ANON_UNION_%lld", ANONYMOUS_UNION_COUNTER++);
				}
			}
			else
			{
				// Parse identifier, use its string value as the datatype's string.
				strcpy_s(type.type_string, token->token->value.strval.length, token->token->value.strval.str);
			}
		}
		else
		{
			// Primitive types - copy the primitive type keyword string into datatype.
			strcpy_s(type.type_string, DATATYPE_MAX_STRING_LEN, keyword_get_string(token->token->value.keywordval));
		}

		// Move to next token.
		token = token->next;
	}
	else if (token->token->type == TOKEN_TYPE_IDENTIFIER)
	{
		// TODO: Support user-defined types / typedefs.
		compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Support for user-defined types is not implemented yet.");
	}
	else
	{
		tree->compiler->position = token->token->position;
		compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Invalid token type %d while parsing data type.", token->token->type);
		return type;
	}	

	// Determine pointer depth by consuming all following POINTER operator tokens.
	while (token != NULL 
		&& token->token->type == TOKEN_TYPE_OPERATOR 
		&& token->token->value.opval == OPERATOR_POINTER)
	{
		type.pointer_depth++;
		token = token->next;
	}

	// Determine type size.
	if (type.pointer_depth > 0)
	{
		type.size = DATATYPE_POINTER_SIZE;
	}
	else if (type.type != DATA_TYPE_STRUCT && type.type != DATA_TYPE_UNION)
	{
		type.size = data_type_primitive_get_size(type.type);
	}
	else
	{
		// TODO: Calculate struct / union size by parsing following tokens.
		// ... or do it later.

		tree->compiler->position = (*token_list)->token->position;
		compiler_error(tree->compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Structured types support is not implemented yet.");
		return type;
	}

	// Once parsing process is done successfully, advance provided token_list pointer to point to the next available parser_token.
	*token_list = token;
	return type;
}

int parse_keyword(struct tree_parsing_context* tree, struct parser_token** start_token)
{
	struct parser_token* token = *start_token;
	if (token == NULL) return -1;

	enum KEYWORD token_keyword = token->token->value.keywordval;
	if (keyword_is_variable_modifier(token_keyword) || keyword_is_datatype(token_keyword))
	{
		// Parsing a variable, a function, a struct, an enum or a union.

		// In all cases, we start with the parsing of a full data type (type of variable, type of structure / union,
		// return type of function).
		struct datatype type = parse_datatype(tree, &token);
		if (compiler_has_error(tree->compiler))
		{
			return -1;
		}
	}

	*start_token = token;

	return 0;
}

int parse_global_keyword(struct tree_parsing_context* tree, struct parser_token** token_list)
{
	int parse_res = parse_keyword(tree, token_list);
	if (parse_res != 0)
	{
		return parse_res;
	}

	// Pop most recently built node which should correspond to a global keyword / symbol (in the C sense) we want to keep track of.
	struct parsing_node* global_node = NULL;
	if (!node_pop(&tree->compiler->node_vec, &tree->compiler->node_tree_vec, global_node))
	{
		return -1;
	}
}

/* 
 * Parses tokens starting from the provided token into a node tree, and returns the root node of that tree.
 * Advances the provided token_list to after all the tokens used by the tree parsed by this run of the function.
 * Returns -1 on error, 0 on successful parse, 1 if token list is empty.
 * 
 * Effectively the "entry point" of the parsing process. Each tree represents a C symbol that constitutes the program:
 * - A function declaration or definition
 * - A global-scope static variable
 * - A user-defined type (structure, enumeration, union or typedef)
 * 
 * The parsing process looks at the next token and attempts to determine which "branch" of the parsing tree to follow,
 * leading to the creation of 1 to many nodes, with the last "pushed" node being the tree's root (and indirectly referencing every node parsed in the process).
 */
static int parse_next_tree(struct compile_process* compiler, struct parser_token** token_list)
{
	struct tree_parsing_context tree = { 0 };
	tree.compiler = compiler;
	tree.root_token = *token_list;

	struct parser_token* parser_token = *token_list;
	if (parser_token == NULL)
	{
		return 1; // No more tokens to parse.
	}

	// Parse the tree's root starting from the first token until we're out of tokens.
	while (parser_token != NULL)
	{
		int res = -1;	
		switch (parser_token->token->type)
		{
			// Expression tree. To be removed later - free-standing expressions in C do not make sense.
		case TOKEN_TYPE_NUMBER:
		case TOKEN_TYPE_OPERATOR:
		case TOKEN_TYPE_IDENTIFIER:
			res = parse_expressionable(&tree, &parser_token);
			if (res == 0) res = 1;
			break;
			// Keyword tree start - static symbol declaration.
		case TOKEN_TYPE_KEYWORD:
			res = parse_global_keyword(&tree, &parser_token);
			break;
		case TOKEN_TYPE_NEWLINE:
			// Skip newlines entirely.
			res = 0;
			parser_token = parser_token->next;
			break;
		}

		if (res < 0)
		{
			compiler->position = (*token_list)->token->position;
			compiler_error(compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Failed to parse new node tree.");
			return -1;
		}
		else if (res == 1) break; // Parsing result of 1 means we've parsed an appropriate tree node.
	}

	// Finished parsing tree. Set the pointer to point to the first token that, in theory, should start the next tree.
	*token_list = parser_token;
	return 0;
}

int parse(struct compile_process* compiler)
{
	struct vector* token_vector = &compiler->token_vec;

	// Perform a pre-pass over the entire token collection, filtering out the types of tokens that are not supported / ignored by the parser.
	struct parsing_node* node = NULL;
	struct parser_token_list token_list = build_parser_token_list(compiler);
	
	struct parser_token* token_list_item = token_list.head;
	if (token_list_item == NULL)
	{
		// No tokens can be parsed.
		// (No need to free the token list since we know it's empty).
		return PARSER_NO_TOKENS;
	}

	// Loop over the entire list of parseable tokens until the end is reached or parsing fails.
	// token_list_item pointer will be "advanced" to the first token of the next potential tree.
	int parse_res = 0;
	while ((parse_res = parse_next_tree(compiler, &token_list_item)) == 0)
	{
	}

	// Handle error. Emit a generic error if none are currently emitted to the compiler but parsing failed.
	if (parse_res != 1)
	{
		if (compiler_has_error(compiler))
		{
			return compiler->stage_error;
		}

		compiler_error(compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Unknown Parser error.");
		return PARSER_GENERAL_ERROR;
	}

	// Report on all parsed tokens.
	printf("Parsed node trees:\n\n");
	for (int i = 0; i < compiler->node_tree_vec.size; i++)
	{
		struct parsing_node* tree_root = *(struct parsing_node**)vector_get_ptr(compiler->node_tree_vec, i);
		print_node(tree_root, 0);
	}

	// Report on parsed scopes / symbols.
	printf("\n\nParsed scopes:\n\n");
	compiler_print_scope_tree(compiler);

	free_parser_token_list(&token_list);
	return PARSER_ALL_OK;
}
