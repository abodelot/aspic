#include "utils.h"

#include <stdarg.h> // va_list
#include <stdio.h>

void* xrealloc(void* pointer, size_t object_size, size_t n)
{
    size_t new_size = object_size * n;
    if (new_size == 0) {
        fprintf(stderr, "FATAL: cannot allocate size of 0\n");
        exit(1);
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) {
        fprintf(stderr, "FATAL: cannot allocate size of %zu\n", new_size);
        exit(1);
    }
    return result;
}

char* alloc_string(size_t length)
{
    char* buffer = malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stderr, "FATAL: cannot allocate string of length %ld\n", length);
        exit(1);
    }
    buffer[length] = '\0';
    return buffer;
}

char* formatstr(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Compute buffer size
    size_t size = vsnprintf(NULL, 0, format, args);

    // Rewind argument list
    va_end(args);
    va_start(args, format);

    // Allocate and write to buffer
    char* buffer = malloc(size + 1);
    vsnprintf(buffer, size + 1, format, args);
    va_end(args);

    return buffer;
}
