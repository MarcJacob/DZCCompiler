#include "compiler.h"
#include <assert.h>

void node_push(struct vector* vec, struct parsing_node* node)
{
	vector_push(*vec, *node, struct parsing_node);
}

struct parsing_node* node_peek(struct vector* vec)
{
	return (struct parsing_node*)vector_back_ptr(vec);
}
