// Simple vector / dynamic array system implementation.

#ifndef VECTOR_INCLUDED
#define VECTOR_INCLUDED

#include <memory.h>
#include <malloc.h>

#include "core_types.h"

// Simple dynamic-array vector using calloc and free to manage memory.
// Holds "items" of a specific size in bytes. Currently does not guarantee alignment (TODO).
// "No safety" policy for now - if you do anything stupid, you will crash ! Perform your own safety checks if anything might go out of bounds.
struct vector
{
	byte* mem; // Start of storage memory.

	ui16 item_size; // Size of each stored item in bytes.
	ui16 capacity; // The current capacity of the vector in number of items.
	ui16 size; // How many items the vector currently contains.
};

// (Re)allocates the memory required by the vector for the specified capacity. 
// If the mem pointer is non-null, will copy existing items then free the previous memory.
inline void vector_realloc(struct vector* vec, ui16 capacity)
{
	byte* prev_mem = vec->mem;
	vec->mem = (byte*)calloc((size_t)capacity, vec->item_size);
	if (!vec->mem)
	{
		// CRASH !
		*((int*)NULL) = 1;
		return;
	}
	if (prev_mem)
	{
		memcpy(vec->mem, prev_mem, vec->capacity * vec->item_size);
		free(prev_mem);
	}

	vec->capacity = capacity;
}

// Creates a new vector for the specified item size and start capacity and returns it by value.
// Usually it's more convenient to use the vector_create macro.
inline struct vector vector_create_val(ui16 item_size, ui16 start_capacity)
{
	struct vector newVector = { 0 };

	newVector.item_size = item_size;
	vector_realloc(&newVector, start_capacity);

	return newVector;
}

// Returns a void pointer to the last item in the vector.
inline void* vector_get_item_ptr(struct vector* vec, ui16 index)
{
	return (void*)(vec->mem + index * vec->item_size);
}

// Pushes item to the vector using a pointer to that item and copying from it into the vector.
// To push by value or a literal, use the vector_push macro.
inline void vector_push_item_ptr(struct vector* vec, void* item)
{
	if (vec->size == vec->capacity)
	{
		// Double capacity on every realloc.
		vector_realloc(vec, vec->capacity * 2);
	}

	void* new_item = vec->mem + vec->size * vec->item_size;
	memcpy(new_item, item, vec->item_size);

	vec->size++;
}

// Removes the last item from the vector.
inline void vector_pop_item(struct vector* vec)
{
	// TODO: Realloc if size gets significantly smaller than capacity ?
	vec->size--;
}

// Frees and resets a vector. Doesn't delete the item_size information so it can be re-allocated later.
inline void vector_free_ptr(struct vector* vec)
{
	if (vec && vec->mem)
	{
		free(vec->mem);
		vec->mem = NULL;
		vec->capacity = 0;
		vec->size = 0;
	}
}

// Creates a new vector and returns it by value.
#define vector_create(item_type, start_capacity)	\
	(vector_create_val(sizeof(item_type), start_capacity))

// Creates a new vector in heap and returns a pointer to it.
#define vector_create_ptr(vecPtr, item_type, start_capacity)	\
	vecPtr = (struct vector*)calloc(1, sizeof(struct vector));	\
	vecPtr->item_size = sizeof(item_type);						\
	vector_realloc(vecPtr, start_capacity)

#define vector_free(name)	\
	(vector_free_ptr(&name))

// Returns the item at the given index by value.
#define vector_get_val(vec, index, item_type)			\
	*((item_type*)vector_get_item_ptr(&vec, index))

// Returns a pointer to the item at the given index.
#define vector_get_ptr(vec, index, item_type)			\
	((item_type*)vector_get_item_ptr(&vec, index))

// Pushes new item by value to the vector. TODO: In Debug builds, assert that the pushed type is the correct size (and hopefully correct type).
#define vector_push(vec, val, item_type)			\
	{ item_type val_wrapper = val;					\
	vector_push_item_ptr(&vec, &val_wrapper); }			

#endif // VECTOR_INCLUDED