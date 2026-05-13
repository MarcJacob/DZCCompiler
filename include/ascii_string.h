// Simple ascii string library, allowing the allocation of a new string and some common operations on it.

#ifndef ASCII_STRING_INCLUDED
#define ASCII_STRING_INCLUDED

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STRING_ASCII_ERROR() { abort(); }	// Error ? Give up and crash the entire process. 
												// This mini string lib won't bother with recovery processes and error handling.

// Primitive safety checking macro that crashes the program if passed a string deemed to be in a non-usable / invalid state.
#define STR_ASCII_SAFETY_CHECKS(ascii_str_ptr)	\
	if (ascii_str_ptr->str == NULL || ascii_str_ptr->mem_size < STRING_ASCII_MEM_ALIGN || ascii_str_ptr->length >= ascii_str_ptr->mem_size)	\
	{																		\
		STRING_ASCII_ERROR();													\
	}

typedef size_t string_size;

static const short			STRING_ASCII_MEM_ALIGN = 8; // The memory alignment to enforce when pushing / shrinking strings.

// Simple string maintaining both a prepended memory size and length, 
// and guarantees a null-termination character for compatibility with C string functions.
struct string_ascii
{
	// Size of the string in memory.
	string_size mem_size;

	// Number of characters in the string, NOT including null-terminator.
	string_size length;

	// Start of the string's memory block.
	char* str;
};

// Returns the real memory size required for the memory block to stay within desired alignment.
// Note: Returns 0 for input size = 0. Do not assume using this will guarantee the use of at least 1 memory block.
inline string_size string_get_aligned_size_ascii(string_size size)
{
	return (size + STRING_ASCII_MEM_ALIGN - 1) / STRING_ASCII_MEM_ALIGN * STRING_ASCII_MEM_ALIGN;
}

// Creates new ASCII string, initialized with optional null-terminated C string.
// Note: Will never create a zero-size string, but at the minimum a "empty" string with a null-terminator character and one aligned block of memory.
inline struct string_ascii string_create_ascii(const char* c_str)
{
	struct string_ascii new_string = { 0 };
	
	string_size in_len = 0;
	if (c_str) { in_len = strlen(c_str); }
	
	new_string.mem_size = string_get_aligned_size_ascii(in_len + 1);
	
	new_string.str = (char*)calloc(1, new_string.mem_size);
	if (!new_string.str)
	{
		STRING_ASCII_ERROR();
	}

	if (c_str && in_len > 0)
	{
		memcpy(new_string.str, c_str, in_len);
		new_string.length = in_len;
	}
	return new_string;
}

// Creates a copy of the string's internal null-terminated string in memory and returns a pointer to it.
inline const char* string_copy_raw_ascii(struct string_ascii* str)
{
	assert(str != NULL);

	const char* copy = malloc(str->length + 1);
	assert(copy != NULL);

	strncpy_s(copy, str->length + 1, str->str, str->length + 1);

	return copy;
}

inline void string_free_ascii(struct string_ascii* str)
{
	if (str->str)
	{
		free(str->str);
		str->str = NULL;
	}
	str->mem_size = 0;
	str->length = 0;
}

// Resizes a string's memory, truncating its content if needed. "capacity" here means "number of useful characters this string can hold".
// The actual new memory size of the string will always be 1 higher than requested to account for a null-termination character.
// Note: If you mean to free the string, call string_free, as this will always result in a string with minimum memory size.
inline void string_set_capacity_ascii(struct string_ascii* str, string_size capacity)
{
	STR_ASCII_SAFETY_CHECKS(str);

	// Ensure size remains at bounds.

	string_size new_size = string_get_aligned_size_ascii(capacity + 1);
	char* new_str = (char*)realloc(str->str, new_size); // Note: It is assumed here that the start of the (potentially new) block is aligned. 
	if (new_str == NULL)
	{
		STRING_ASCII_ERROR();
	}

	str->str = new_str;

	if (new_size < str->mem_size)
	{
		// Truncation:
		// Make sure last character is null-terminator, truncating string if need be.
		str->length = new_size - 1;
		str->str[str->length] = '\0';
	}
	// If memory was expanded, nothing needs to be done since it is assumed the original string was correctly formed and already has a null-terminator in place.

	str->mem_size = new_size;
}

// Starts from the earliest found null-terminator and appends the passed in characters.
// The string will be resized to fit the new characters if needed.
inline void string_append_ascii(struct string_ascii* str, const char* c_str)
{
	STR_ASCII_SAFETY_CHECKS(str);

	string_size appended_len = strlen(c_str);
	if (appended_len == 0) return; // ... Really ?

	// Copy what we can from the appended character into existing memory, and push the rest if needed.
	string_size copy_size = str->mem_size - 1 - str->length;
	if (copy_size > appended_len)
	{
		copy_size = appended_len;
	}

	if (copy_size > 0)
	{
		memcpy(str->str + str->length, c_str, copy_size);
		str->length += copy_size;
	}

	if (copy_size < appended_len)
	{
		string_size remaining = appended_len - copy_size;
		string_set_capacity_ascii(str, str->length + remaining);
		memcpy(str->str + str->length, c_str + copy_size, remaining);
		str->length += remaining;
	}

	// Ensure last character is null-terminator.
	str->str[str->length] = '\0';
}

// Appends a single ASCII character to the string.
inline void string_append_char_ascii(struct string_ascii* str, char c)
{
	char totally_a_string[2];
	totally_a_string[0] = c;
	totally_a_string[1] = '\0';

	string_append_ascii(str, totally_a_string);
}

#endif // ASCII_STRING_INCLUDED