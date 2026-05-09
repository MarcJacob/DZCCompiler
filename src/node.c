#include "compiler.h"
#include <assert.h>

#define NODE_PRINT_MAX_INDENTATION (256)

void print_node(struct parsing_node* node, int indentation)
{
	const char indents[NODE_PRINT_MAX_INDENTATION];

	if (indentation >= NODE_PRINT_MAX_INDENTATION)
	{
		indentation = NODE_PRINT_MAX_INDENTATION;
	}

	memset(indents, '\t', indentation);
	memset(indents + indentation, 0, NODE_PRINT_MAX_INDENTATION - indentation);

	if (node == NULL)
	{
		printf("%s<NONE>\n", indents);
		return;
	}
	else if (indentation == NODE_PRINT_MAX_INDENTATION)
	{
		printf("%s<...>\n", indents);
		return;
	}

	switch (node->type)
	{
	case NODE_TYPE_EXPRESSION:
		printf("%s<EXPRESSION, (%s)>\n", indents, op_get_string(node->value.exp.op));
		print_node(node->value.exp.left, indentation + 1);
		print_node(node->value.exp.right, indentation + 1);
		break;
	case NODE_TYPE_NUMBER:
		printf("%s<NUMBER, %lld>\n", indents, node->value.llnum);
		break;
	default:
		printf("%s<UNKNOWN>\n", indents);
	}
}

void node_push(struct vector* vec, struct parsing_node* node)
{
	vector_push(*vec, *node);
}

struct parsing_node* node_peek(struct vector* node_vec)
{
	return vector_back_ptr(node_vec);
}

int node_pop(struct vector* node_vec, struct vector* root_node_vec, struct parsing_node* out_popped)
{
	struct parsing_node* last_node = vector_back_ptr(node_vec);
	struct parsing_node** last_root_node = last_node != NULL ? vector_back_ptr(root_node_vec) : NULL;

	if (last_node == NULL) return 0;

	// Pop the root node vector if we're popping a root node from the main node vector.
	if (last_root_node != NULL && last_node == *last_root_node)
	{
		vector_pop_item(root_node_vec);
	}

	if (out_popped)
	{
		// Copy last_node into out_popped.
		memcpy(out_popped, last_node, sizeof(struct parsing_node));
	}

	// Pop the node vector, freeing last_node.
	vector_pop_item(node_vec);

	return 1;
}

