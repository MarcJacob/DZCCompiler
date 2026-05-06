#include "compiler.h"

int parse_next(struct compile_process* compiler)
{
	return 0;
}

int parse(struct compile_process* compiler)
{
	struct vector* token_vector = &compiler->token_vec;

	struct parsing_node* node = NULL;

	while (parse_next(compiler) == 0)
	{
		node = node_peek(&compiler->node_vec);

		if (!node)
		{
			break;
		}
		vector_push(compiler->node_tree_vec, *node, struct parsing_node);
	}

	return PARSER_ALL_OK;
}
