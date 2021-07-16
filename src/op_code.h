#ifndef ASPIC_OP_CODE_H
#define ASPIC_OP_CODE_H

#include "value.h"

// Each instruction starts with 1 byte operation code, defined as OpCode.
typedef enum {
    OP_RETURN,
    OP_POP,

    // Jumps (2 bytes operand: offset)
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_JUMP_BACK,

    // Global variables (1 byte operand: constant index)
    OP_DECL_GLOBAL,
    OP_DECL_GLOBAL_CONST,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    // Global variables (2 bytes operand: constant index)
    OP_DECL_GLOBAL_16,
    OP_DECL_GLOBAL_CONST_16,
    OP_GET_GLOBAL_16,
    OP_SET_GLOBAL_16,

    // Local variables (1 byte operand: stack offset)
    OP_GET_LOCAL,
    OP_SET_LOCAL,

    // Literals (1 byte and 2 bytes operand: constant index)
    OP_CONSTANT,
    OP_CONSTANT_16,

    // Predefined constants
    OP_ZERO,
    OP_ONE,
    OP_TRUE,
    OP_FALSE,
    OP_NULL,

    // Unary operators
    OP_NOT,
    OP_POSITIVE,
    OP_NEGATIVE,

    // Binary operators
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,

    // Comparator operators
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,

    // Subscript operator []
    OP_SUBSCRIPT_GET,
    OP_SUBSCRIPT_SET,

    // Function call ()
    OP_CALL,

    // Array expression [] (1 byte operand: item count)
    OP_ARRAY,
} OpCode;

// Convert enum to string, for debug purpose
const char* op2str(OpCode);

// OP_NOT: !value
Value op_not(Value value);

// OP_POSITIVE: +value
Value op_positive(Value value);

// OP_NEGATIVE: -value
Value op_negative(Value value);

// OP_ADD: a + b
Value op_add(Value b, Value a);

// OP_SUBTRACT: a - b
Value op_subtract(Value b, Value a);

// OP_MULTIPLY: a * b
Value op_multiply(Value b, Value a);

// OP_DIVIDE: a / b
Value op_divide(Value b, Value a);

// OP_MODULO: a % b
Value op_modulo(Value b, Value a);

// OP_GREATER: a > b
Value op_greater(Value b, Value a);

// OP_GREATER_EQUAL: a >= b
Value op_greater_equal(Value b, Value a);

// OP_SUBSCRIPT_GET: collection[index]
Value op_subscript_get(Value collection, Value index);

// OP_SUBSCRIPT_SET: collection[index] = value
Value op_subscript_set(Value collection, Value index, Value value);

#endif
