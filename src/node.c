#include "compiler.h"
#include <assert.h>

#define NODE_PRINT_MAX_INDENTATION (256)

// Tries to calculate the arithmetic result of an expression node, with all identifiers considered equal to 0.
// TODO: Move this elsewhere so that context can be provided in case the identifiers themselves have a compile-time-resolvable value.
// TODO: Support non-integers.
int calculate_expression_node_result(struct parsing_node* root_exp)
{
	int left, right;

	switch (root_exp->value.exp.left->type)
	{
	case NODE_TYPE_EXPRESSION:
		left = calculate_expression_node_result(root_exp->value.exp.left);
		break;
	case NODE_TYPE_NUMBER:
		left = (int)root_exp->value.exp.left->value.llnum;
		break;
	default:
		left = 0;
	}

	switch (root_exp->value.exp.right->type)
	{
	case NODE_TYPE_EXPRESSION:
		right = calculate_expression_node_result(root_exp->value.exp.right);
		break;
	case NODE_TYPE_NUMBER:
		right = (int)root_exp->value.exp.right->value.llnum;
		break;
	default:
		right = 0;
	}

	switch (root_exp->value.exp.op)
	{
	case OPERATOR_ADD:
		return left + right;
	case OPERATOR_SUB:
		return left - right;
	case OPERATOR_MULT:
		return left * right;
	case OPERATOR_DIV:
		if (right == 0) return (~0);
		return left / right;
	case OPERATOR_MOD:
		if (right == 0) return (~0);
		return left % right;
	case OPERATOR_POW:
		if (right == 0) return 1;
		for (int i = 0; i < right; i++)
		{
			left *= left;
		}
		return left;
	default:
		return 0;
	}
}

void print_node(struct parsing_node* node, int indentation)
{
	if (indentation >= NODE_PRINT_MAX_INDENTATION)
	{
		indentation = NODE_PRINT_MAX_INDENTATION;
	}

	if (node == NULL)
	{
		printf("%*s<NONE>\n", indentation * 4, "");
		return;
	}
	else if (indentation == NODE_PRINT_MAX_INDENTATION)
	{
		printf("%*s<...>\n", indentation * 4, "");
		return;
	}

	switch (node->type)
	{
	case NODE_TYPE_EXPRESSION:
		printf("%*s<EXPRESSION, (OP %s) (Depth = %d)> RESULT = %d\n", indentation * 4, "", op_get_string(node->value.exp.op), node->value.exp.parenthesis_level, calculate_expression_node_result(node));
		print_node(node->value.exp.left, indentation + 1);
		print_node(node->value.exp.right, indentation + 1);
		break;
	case NODE_TYPE_NUMBER:
		printf("%*s<NUMBER, %lld>\n", indentation * 4, "", node->value.llnum);
		break;
	default:
		printf("%*s<NONE>\n", indentation * 4, "");
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

