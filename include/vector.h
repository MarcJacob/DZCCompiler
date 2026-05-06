// Simple vector / dynamic array system implementation.

#ifndef VECTOR_INCLUDED
#define VECTOR_INCLUDED

#include <memory.h>
#include <malloc.h>
#include <assert.h>

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
	assert(vec != NULL);

	byte* prev_mem = vec->mem;

	vec->mem = (byte*)calloc((size_t)capacity, vec->item_size);
	assert(vec->mem != NULL);

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
	assert(item_size > 0);

	struct vector newVector = { 0 };

	newVector.item_size = item_size;
	vector_realloc(&newVector, start_capacity);

	return newVector;
}

// Returns a void pointer to the last item in the vector.
inline void* vector_get_item_ptr(struct vector* vec, ui16 index)
{
	assert(vec != NULL && vec->size > index);
	return (void*)(vec->mem + index * vec->item_size);
}

// Pushes item to the vector using a pointer to that item and copying from it into the vector.
// To push by value or a literal, use the vector_push macro.
inline void vector_push_item_ptr(struct vector* vec, void* item)
{
	assert(vec != NULL && item != NULL);

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
	assert(vec != NULL && vec->size > 0);
	vec->size--;
}

// Frees and resets a vector. Doesn't delete the item_size information so it can be re-allocated later.
inline void vector_free_ptr(struct vector* vec)
{
	assert(vec != NULL);

	if (vec && vec->mem)
	{
		free(vec->mem);
		vec->mem = NULL;
		vec->capacity = 0;
		vec->size = 0;
	}
}

inline struct vector vector_create_copy(struct vector* source_vec)
{
	assert(source_vec != NULL);

	struct vector new_vec = vector_create_val(source_vec->item_size, source_vec->size);
	new_vec.size = source_vec->size;
	memcpy(new_vec.mem, source_vec->mem, new_vec.size * new_vec.item_size);

	return new_vec;
}

inline void* vector_back_ptr(struct vector* vec)
{
	assert(vec != NULL);

	if (vec->size == 0) return NULL;

	return vec->mem + vec->size - 1;
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
#define vector_get_ptr(vec, index)			\
	vector_get_item_ptr(&vec, index)

// Pushes new item by value to the vector. val_type determines the actual type that will be pushed into the vector.
// Use this to push a rvalue, or to perform an implicit conversion while pushing.
// The type must be of the same size as the vector's item size.
#define vector_push_val(vec, val, val_type)			\
	assert(sizeof(val_type) == (vec).item_size);	\
	{ val_type val_wrapper = val;					\
	vector_push_item_ptr(&(vec), &val_wrapper); }			

// Pushes new item by value to the vector. Uses the value's address so it only works with lvalues.
// The value's type must match the vector's item size.
#define vector_push(vec, val)				\
	assert(sizeof(val) == (vec).item_size);	\
	vector_push_item_ptr(&(vec), &val);

#endif // VECTOR_INCLUDED