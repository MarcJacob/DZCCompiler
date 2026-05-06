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

struct parsing_node* node_pop(struct vector* node_vec, struct vector* root_node_vec)
{
	struct parsing_node* last_node = vector_back_ptr(node_vec);
	struct parsing_node* last_node_root = node_vec->size > 0 ? vector_back_ptr(root_node_vec) : NULL;

	vector_pop_item(node_vec);

	// Pop the root vector if we've just popped a root node from the main node vector.
	if (last_node == last_node_root)
	{
		vector_pop_item(root_node_vec);
	}

	return last_node;
}
