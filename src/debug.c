#include "debug.h"

#include <stdio.h>

void chunk_dump(const Chunk* chunk, const char* name)
{
    printf("== %s::data ==\n", name);
    for (int i = 0; i < chunk->constants.count; ++i) {
        printf("[%d] ", i);
        value_repr(chunk->constants.values[i]);
        printf("\n");
    }

    printf("== %s::bytecode ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = instruction_dump(chunk, offset);
    }
}

static int instruction_noarg(const char* name, int offset)
{
    puts(name);
    return offset + 1;
}

static int instruction_byte(const char* name, const Chunk* chunk, int offset)
{
    uint8_t byte = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, byte);
    return offset + 2;
}

static int instruction_with_constant(const char* name, const Chunk* chunk, int offset)
{
    uint8_t constant_idx = chunk->code[offset + 1];
    printf("%-16s %4d -> ", name, constant_idx);
    value_repr(chunk->constants.values[constant_idx]);
    printf("\n");

    return offset + 2; // 1 byte op code + 1 byte operand (constant index)
}

static int instruction_with_constant_16(const char* name, const Chunk* chunk, int offset)
{
    uint16_t constant_idx = chunk->code[offset + 1] << 8 | chunk->code[offset + 2];
    printf("%-16s %4d ", name, constant_idx);
    value_repr(chunk->constants.values[constant_idx]);
    printf("\n");

    return offset + 3; // 1 byte op code + 2 bytes operand (constant index)
}

static int instruction_jump(const char* name, int sign, const Chunk* chunk, int offset)
{
    uint16_t jump = chunk->code[offset + 1] << 8 | chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);

    return offset + 3; // 1 byte op + 2 bytes operand (jump offset)
}

int instruction_dump(const Chunk* chunk, int offset)
{
    printf("%04d ", offset);

    // Print line number. Do not print it if same than previous.
    const int lineno = chunk_get_line(chunk, offset);
    if (offset > 0 && lineno == chunk_get_line(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", lineno);
    }

    uint8_t instruction = chunk->code[offset];
    const char* desc = op2str(instruction);

    switch (instruction) {
    case OP_RETURN:
    case OP_POP:
        return instruction_noarg(desc, offset);

    // Jumps
    case OP_JUMP:
    case OP_JUMP_IF_TRUE:
    case OP_JUMP_IF_FALSE:
        return instruction_jump(desc, 1, chunk, offset);
    case OP_JUMP_BACK:
        return instruction_jump(desc, -1, chunk, offset);

    // Global variables
    case OP_DECL_GLOBAL:
    case OP_DECL_GLOBAL_CONST:
    case OP_GET_GLOBAL:
    case OP_SET_GLOBAL:
        return instruction_with_constant(desc, chunk, offset);
    case OP_DECL_GLOBAL_16:
    case OP_DECL_GLOBAL_CONST_16:
    case OP_GET_GLOBAL_16:
    case OP_SET_GLOBAL_16:
        return instruction_with_constant_16(desc, chunk, offset);

    // Local variables
    case OP_GET_LOCAL:
    case OP_SET_LOCAL:
        return instruction_byte(desc, chunk, offset);

    // Literals
    case OP_CONSTANT:
        return instruction_with_constant(desc, chunk, offset);
    case OP_CONSTANT_16:
        return instruction_with_constant_16(desc, chunk, offset);

    // Predefined literals
    case OP_ZERO:
    case OP_ONE:
    case OP_TRUE:
    case OP_FALSE:
    case OP_NULL:

    // Unary operators
    case OP_NOT:
    case OP_POSITIVE:
    case OP_NEGATIVE:

    // Binary operators
    case OP_ADD:
    case OP_SUBTRACT:
    case OP_MULTIPLY:
    case OP_DIVIDE:
    case OP_MODULO:

    // Comparator operators
    case OP_EQUAL:
    case OP_NOT_EQUAL:
    case OP_GREATER:
    case OP_GREATER_EQUAL:
    case OP_LESS:
    case OP_LESS_EQUAL:
        return instruction_noarg(desc, offset);

    // Subscript operator []
    case OP_SUBSCRIPT_GET:
    case OP_SUBSCRIPT_SET:
        return instruction_noarg(desc, offset);

    // Call operator ()
    case OP_CALL:
        return instruction_byte(desc, chunk, offset);

    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
