#include "utils.h"

#include <stdio.h>
#include <string.h> // strchr

void* realloc_array(void* pointer, size_t object_size, size_t n)
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

void print_line(FILE* stream, const char* buffer, int line)
{
    // Find the first \n
    const char* start = buffer;
    for (int i = 1; i < line; ++i) {
        start = strchr(start, '\n') + 1;
    }

    // Consume whitespaces
    while (*start == ' ') {
        ++start;
    }

    // Fint the second \n
    const char* stop = strchr(start, '\n');
    if (stop != NULL) {
        // Print everything in between
        fprintf(stream, "%.*s\n", (int)(stop - start), start);
    } else {
        fprintf(stream, "%s\n", start);
    }
}
