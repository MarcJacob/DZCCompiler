#include "compiler.h"

struct parser_token_list
{
	struct parser_token_list_item
	{
		struct token* token;
		struct parser_token_list_item* next;
	} *head;
};

// Builds a linked list of pointers to tokens the parser knows how to parse.
static struct parser_token_list build_parser_token_list(struct compile_process* compiler)
{
	struct parser_token_list list = { 0 };
	struct parser_token_list_item* item = NULL;

	// Go through each token and check their type. Only certain types are supported.
	for (int token_index = 0; token_index < compiler->token_vec.size; token_index++)
	{
		struct token* token = vector_get_ptr(compiler->token_vec, token_index, struct token);
		struct parser_token_list_item* new_item = NULL;
		switch (token->type)
		{
		case TOKEN_TYPE_NUMBER:
		case TOKEN_TYPE_IDENTIFIER:
		case TOKEN_TYPE_STRING:
			new_item = calloc(1, sizeof(struct parser_token_list_item));
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
	struct parser_token_list_item* item = list->head;
	list->head = NULL;
	while (item->next)
	{
		struct parser_token_list_item* previous = item;
		item = item->next;
		free(previous);
	}

	// Free last item.
	free(item);
}

static struct parsing_node* parse_single_token_node(struct token* token)
{
	struct parsing_node* new_node = calloc(1, sizeof(struct parsing_node));
	assert(new_node != NULL);

	switch (token->type)
	{
	case TOKEN_TYPE_NUMBER:
		new_node->type = NODE_TYPE_NUMBER;
		new_node->value.llnum = token->value.llnum;
		break;
	case TOKEN_TYPE_IDENTIFIER:
		new_node->type = NODE_TYPE_IDENTIFIER;
		new_node->value.sval = token->value.strval;
		break;
	case TOKEN_TYPE_STRING:
		new_node->type = NODE_TYPE_STRING;
		new_node->value.sval = token->value.strval;
		break;
	default:
		return NULL; // Return NULL as a signal that the passed in token is not of a type that can be parsed to a single-token node.
	}

	new_node->position = token->position;
	return new_node;
}

int parse_token(struct compile_process* compiler, struct token* token)
{
	struct parsing_node* new_node = NULL;
	switch (token->type)
	{
	case TOKEN_TYPE_NUMBER:
	case TOKEN_TYPE_IDENTIFIER:
	case TOKEN_TYPE_STRING:
		new_node = parse_single_token_node(token);
		if (!new_node)
		{
			compiler->position = token->position;
			compiler_error(compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Failed to parse single-token node.");
			return -1;
		}
		break;
	}

	if (!new_node)
	{
		compiler->position = token->position;
		compiler_error(compiler, COMPILER_PARSER_ERROR, PARSER_GENERAL_ERROR, "Failed to parse token.");
		return -1;
	}

	// Append newly-parsed node (by copy) to compiler node vector.
	vector_push(compiler->node_vec, *new_node);
	free(new_node);
	new_node = vector_back_ptr(&compiler->node_vec);

	return 0;
}

int parse(struct compile_process* compiler)
{
	struct vector* token_vector = &compiler->token_vec;

	struct parsing_node* node = NULL;
	struct parser_token_list token_list = build_parser_token_list(compiler);
	
	struct parser_token_list_item* token_list_item = token_list.head;

	// Loop over the entire list of parseable tokens until the end is reached or parsing fails.
	while (token_list_item && parse_token(compiler, token_list_item->token) == 0)
	{
		node = node_peek(&compiler->node_vec);
		vector_push(compiler->node_tree_vec, *node);

		// Go to next parseable token.
		token_list_item = token_list_item->next;
	}

	free_parser_token_list(&token_list);
	return PARSER_ALL_OK;
}
