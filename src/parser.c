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

// Copies the passed node into the tree's compiler's node vector, and frees the node.
// Also populates the node's context.
static void push_node_to_tree(struct tree_parsing_context* tree, struct parsing_node* node)
{
	vector_push(tree->compiler->node_vec, *node);
	free(node);

	// TODO: Set node context.
}

// Creates a new node from the passed in token, if the token is of a compatible type.
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
struct parsing_node* build_expression_node(struct parsing_node* left_operand, struct parsing_node* right_operand, enum OPERATOR_TYPE op, int parenthesis_level)
{
	assert(left_operand != NULL);
	assert(right_operand != NULL);

	// Build new "original parent" node with the specific input operator and original precedence over the two input operands.
	struct parsing_node* exp_node = calloc(1, sizeof(struct parsing_node));
	assert(exp_node != NULL);

	exp_node->type = NODE_TYPE_EXPRESSION;

	exp_node->value.exp.left = left_operand;
	exp_node->value.exp.right = right_operand;
	exp_node->value.exp.op = op;

	exp_node->value.exp.parenthesis_level = parenthesis_level;

	// Handle operator precedence.
	// If the right operand is an expression node with higher precedence and the same parenthesis level, then

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

	printf("Parsing expression node around operator %s...\n", op_get_string(token->token->value.opval));

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

	printf("Parsed expression node around operator %s\n", op_get_string(token->token->value.opval));

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

		if (parser_token == NULL) // Break out when reaching something that must end the expression.
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

// Parses tokens starting from the provided token into a node tree, and returns the root node of that tree.
// Advances the provided token_list to after all the tokens used by the tree parsed by this run of the function.
// Returns -1 on error, 0 on successful parse, 1 if token list is empty.
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
	// TODO: Currently we only know how to parse expressionables without more context. Later expressionables will not be able to be valid tree roots,
	// and instead of continuously parsing tokens we'll instead look for valid roots and stop once the recursive parsing process for that node is done.
	{
		int res = parse_expressionable(&tree, &parser_token);
		// TODO: Add non-expressionable nodes.

		if (res != 0)
		{
			compiler->position = parser_token->token->position;
			compiler_error(compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Failed to parse new node tree.");
			return -1;
		}

		// TODO: Break out of the loop if we've parsed a valid tree node. 
	}

	// Finished parsing tree. Set the pointer to point to the first token that, in theory, should start the next tree.
	*token_list = parser_token;
	return 0;
}

int parse(struct compile_process* compiler)
{
	struct vector* token_vector = &compiler->token_vec;

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
		node = node_peek(&compiler->node_vec);
		vector_push(compiler->node_tree_vec, node);
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
		struct parsing_node* tree_root = vector_get_ptr(compiler->node_vec, i);
		print_node(tree_root, 0);
	}

	free_parser_token_list(&token_list);
	return PARSER_ALL_OK;
}
