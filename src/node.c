#include "compiler.h"
#include <assert.h>

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

