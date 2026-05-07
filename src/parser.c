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

struct history
{
	int flags;
};

static struct history* history_begin(int flags)
{
	struct history* new_history = calloc(1, sizeof(struct history));
	assert(new_history != NULL);

	new_history->flags = flags;
	return new_history;
}

static struct history* history_down(struct history* history)
{
	struct history* new_history = calloc(1, sizeof(struct history));
	assert(new_history != NULL);
	memcpy(new_history, history, sizeof(struct history));

	return new_history;
}

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
	struct history* history;
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
struct parsing_node* build_expression_node(struct parsing_node* left_operand, struct parsing_node* right_operand, const char* op_str)
{
	assert(left_operand != NULL);
	assert(right_operand != NULL);

	struct parsing_node* exp_node = calloc(1, sizeof(struct parsing_node));
	assert(exp_node != NULL);

	exp_node->type = NODE_TYPE_EXPRESSION;

	exp_node->value.exp.left = left_operand;
	exp_node->value.exp.right = right_operand;
	exp_node->value.exp.op_str = op_str;

	return exp_node;
}

static int parse_expressionable(struct tree_parsing_context* tree, struct parser_token** start_token);

static int parse_expressionable_for_op(struct tree_parsing_context* tree, struct parser_token** start_token, const char* op_str)
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
	left_operand->flags |= NODE_FLAG_INSIDE_EXPRESSION;

	// Now parse next node and attempt to use it as right operand.
	struct parser_token* right_operand_start_token = token->next;
	const char* op_str = token->token->value.strval.str;
	if (parse_expressionable_for_op(tree, &right_operand_start_token, op_str) != 0)
	{
		// Right token was not an expressionable.
		free(left_operand);
		return -1;
	}

	// Extract parsed node from the node vector.
	struct parsing_node* right_operand = calloc(1, sizeof(struct parsing_node));
	assert(right_operand != NULL);
	node_pop(&tree->compiler->node_vec, &tree->compiler->node_tree_vec, right_operand);
	right_operand->flags |= NODE_FLAG_INSIDE_EXPRESSION;

	struct parsing_node* exp_node = build_expression_node(left_operand, right_operand, op_str);

	// TODO: Reorder the expression.

	push_node_to_tree(tree, exp_node);

	// Update start_token pointer to point to next token.
	// After parsing the right operand, its start token pointer should be equal to the token coming after.
	*start_token = right_operand_start_token;

	return 0;
}

// An "expressionable" is a node that can form an expression with an operator and other expressionables.
static int parse_expressionable(struct tree_parsing_context* tree, struct parser_token** start_token)
{
	// Cache history and have the tree use a copy for the scope of this function.
	struct history* prev_history = tree->history;
	tree->history = history_down(prev_history);

	struct parser_token* parser_token = *start_token;
	struct parsing_node* node = NULL;
	int parse_res = -1;

	tree->history->flags |= NODE_FLAG_INSIDE_EXPRESSION;

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
	
	if (parse_res == 0)
	{
		// Update start_token pointer to point to next token.
		*start_token = parser_token;
	}

	// Free scope history and restore prev history to tree.
	free(tree->history);
	tree->history = prev_history;

	return parse_res;
}

// Parses tokens starting from the provided list item into a node tree, and returns the root node of that tree.
// Advances the provided tree_start_token to after all the tokens used by the tree parsed by this run of the function.
// Returns -1 on error, 0 on successful parse, 1 if token list is empty.
static int parse_next_tree(struct compile_process* compiler, struct parser_token** tree_start_token)
{
	struct history* parsing_history = history_begin(0);

	struct tree_parsing_context tree = { 0 };
	tree.compiler = compiler;
	tree.history = parsing_history;
	tree.root_token = *tree_start_token;

	struct parser_token* parser_token = *tree_start_token;
	if (parser_token == NULL)
	{
		return 1; // No more tokens to parse.
	}

	while (parser_token)
	{
		int res = parse_expressionable(&tree, &parser_token);
		// TODO: Add non-expressionable nodes.

		if (res != 0)
		{
			free(parsing_history);

			compiler->position = parser_token->token->position;
			compiler_error(compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Failed to parse token.");
			return -1;
		}
	}

	// Finished parsing tree. Set the pointer to point to the first token that, in theory, should start the next tree.
	*tree_start_token = parser_token;

	free(parsing_history);
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
	// TODO: Make a print function for nodes so that it can be recursively re-used.
	printf("Parsed nodes:\n\n");
	for (int i = 0; i < compiler->node_vec.size; i++)
	{
		struct parsing_node* node = vector_get_ptr(compiler->node_vec, i);
		switch (node->type)
		{
		case NODE_TYPE_EXPRESSION:
			printf("<EXPRESSION, %s>\n", node->value.exp.op_str);
			printf("\t<NUMBER, %lld>\n", node->value.exp.left->value.llnum);
			printf("\t<NUMBER, %lld>\n", node->value.exp.right->value.llnum);
			break;
		case NODE_TYPE_NUMBER:
			printf("\t<NUMBER, %lld>\n", node->value.llnum);
			break;
		default:
			printf("<UNKNOWN>\n");
		}
	}

	free_parser_token_list(&token_list);
	return PARSER_ALL_OK;
}
