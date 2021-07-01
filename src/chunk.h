#ifndef ASPIC_CHUNK_H
#define ASPIC_CHUNK_H

#include "op_code.h"
#include "value.h"
#include "value_array.h"
#include "shared.h"

typedef struct {
    int count;
    int capacity;
    int* values;
} IntArray;

typedef struct {
    // Dynamic array of instructions
    int count;
    int capacity;
    uint8_t* code;

    // Constants to be loaded in this chunk
    ValueArray constants;

    // Line numbers
    IntArray lines;

    // Source code buffer
    const char* source;
} Chunk;

/**
 * Ctor
 * @param source: source code buffer
 */
void chunk_init(Chunk* chunk, const char* source);

/**
 * Dtor
 */
void chunk_free(Chunk* chunk);

/**
 * Append a byte at the end of the chunk
 * @param lineno: line number from source
 */
void chunk_write(Chunk* chunk, uint8_t byte, int lineno);

/**
 * Write the OpCode to load a constant, followed by the index value of the constant.
 * If index fits on 1 byte, write OP_CONSTANT.
 * If index fits on 2 bytes, write OP_CONSTANT_16.
 * @param index: the constant index value, from chunk_register_constant (max: 2^16 - 1)
 * @param lineno: line number from source
 * @return true if success
 */
bool chunk_write_constant(Chunk* chunk, unsigned int index, int lineno);

/**
 * Add a new constant to the chunk
 * @return the index where the constant was appended
 */
unsigned int chunk_register_constant(Chunk*, Value value);

/**
 * Get line number for the given instruction offset
 */
int chunk_get_line(const Chunk* chunk, size_t offset);

/**
 * Print given line from the source buffer
 */
void chunk_print_line(const Chunk* chunk, int line);

#endif
