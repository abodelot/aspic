#include "chunk.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

static void int_array_init(IntArray* array)
{
    array->count = array->capacity = 0;
    array->values = 0;
}

static void int_array_free(IntArray* array)
{
    if (array->capacity != 0) {
        free(array->values);
        array->values = NULL;
    }

    array->count = array->capacity = 0;
}

static void int_array_push(IntArray* array, int value)
{
    if (array->capacity < array->count + 1) {
        array->capacity = array->capacity < 8 ? 8 : array->capacity * 2;
        array->values = xrealloc(array->values, sizeof(int), array->capacity);
    }

    array->values[array->count++] = value;
}

void chunk_init(Chunk* chunk, const char* source)
{
    chunk->count = chunk->capacity = 0;
    chunk->code = NULL;
    chunk->source = source;

    value_array_init(&chunk->constants);
    int_array_init(&chunk->lines);
}

void chunk_free(Chunk* chunk)
{
    // Free 'code' array
    if (chunk->capacity != 0) {
        free(chunk->code);
        chunk->code = NULL;
    }
    chunk->count = chunk->capacity = 0;

    value_array_free(&chunk->constants);
    int_array_free(&chunk->lines);
}

void chunk_write(Chunk* chunk, uint8_t byte, int lineno)
{
    if (chunk->capacity < chunk->count + 1) {
        chunk->capacity = chunk->capacity < 8 ? 8 : chunk->capacity * 2;
        chunk->code = xrealloc(chunk->code, sizeof(uint8_t), chunk->capacity);
    }
    chunk->code[chunk->count++] = byte;

    // Because each line contains multiple op codes, line numbers are compressed
    // using run-length encoding: https://en.wikipedia.org/wiki/Run-length_encoding
    //
    // The list of following line numbers:
    // 2 2 2 3 3 3 3 3 5 6 6 6 6
    //
    // Is compressed and stored as follow in chunk->lines.values:
    // 3 2  5 3  1 5  4 6
    //
    // Each pair reads as: (count, lineno), (count, lineno), ...

    // If last lineno equals last known line number
    if (chunk->lines.count > 0 && chunk->lines.values[chunk->lines.count - 1] == lineno) {
        // Increment counter
        chunk->lines.values[chunk->lines.count - 2]++;
    } else {
        // Append pair (1, lineno) to array
        int_array_push(&chunk->lines, 1);
        int_array_push(&chunk->lines, lineno);
    }
}

bool chunk_write_constant(Chunk* chunk, unsigned int index, int lineno)
{
    if (index <= UINT8_MAX) {
        chunk_write(chunk, OP_CONSTANT, lineno);
        chunk_write(chunk, index, lineno);
        return true;
    }
    if (index <= UINT16_MAX) {
        chunk_write(chunk, OP_CONSTANT_16, lineno);
        // Convert constant index to a 2-bytes integer
        chunk_write(chunk, (index >> 8) & 0xff, lineno);
        chunk_write(chunk, index & 0xff, lineno);
        return true;
    }
    // Unsupported, index cannot fit on 2 bytes
    return false;
}

unsigned int chunk_register_constant(Chunk* chunk, Value value)
{
    // Check if value is already registered
    int index = value_array_find(&chunk->constants, value);
    if (index >= 0) {
        return index;
    }
    // Store value and return index
    value_array_push(&chunk->constants, value);
    return chunk->constants.count - 1;
}

int chunk_get_line(const Chunk* chunk, size_t offset)
{
    size_t current_offset = 0;
    // Loop over each pair
    for (int i = 0; i < chunk->lines.count; i += 2) {
        current_offset += chunk->lines.values[i];
        if (current_offset > offset) {
            return chunk->lines.values[i + 1];
        }
    }
    return 0;
}

void chunk_print_line(const Chunk* chunk, int line)
{
    // Find the first \n
    const char* start = chunk->source;
    for (int i = 1; i < line; ++i) {
        start = strchr(start, '\n') + 1;
    }

    // Fint the second \n
    const char* stop = strchr(start, '\n');
    if (stop != NULL) {
        // Print everything in between
        fprintf(stderr, "%.*s\n", (int)(stop - start), start);
    } else {
        fprintf(stderr, "%s\n", start);
    }
}
