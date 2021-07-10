#ifndef ASPIC_UTILS_H
#define ASPIC_UTILS_H

#include "shared.h"

/**
 * (Re)allocate n * object_size. Use realloc.
 * Exit program immediately if allocation failed.
 */
void* xrealloc(void* pointer, size_t object_size, size_t n);

/**
 * Allocate buffer of size length+1 with last char set at \0.
 * Exit program immediately if allocation failed.
 */
char* alloc_string(size_t length);

/**
 * Dynamically allocate and format a string.
 * @return allocated buffer.
 */
char* formatstr(const char* format, ...);

#endif
