#ifndef ASPIC_OP_CODE_H
#define ASPIC_OP_CODE_H

#include "value.h"

// Each instruction starts with 1 byte operation code, defined as OpCode.
typedef enum {
    OP_CONSTANT,    // 1 byte operand
    OP_DECL_GLOBAL,
    OP_DECL_GLOBAL_CONST,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,

    OP_CONSTANT_16, // 2 bytes operand
    OP_DECL_GLOBAL_16,
    OP_DECL_GLOBAL_CONST_16,
    OP_GET_GLOBAL_16,
    OP_SET_GLOBAL_16,

    OP_RETURN,
    OP_POP,


    // Predefined constans
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

    // Function call operator
    OP_CALL,
} OpCode;

// Convert enum to string, for debug purpose
const char* op2str(OpCode);

// OP_NOT
Value op_not(Value value);

// OP_POSITIVE
Value op_positive(Value value);

// OP_NEGATIVE
Value op_negative(Value value);

// OP_ADD
Value op_add(Value b, Value a);

// OP_SUBTRACT
Value op_subtract(Value b, Value a);

// OP_MULTIPLY
Value op_multiply(Value b, Value a);

// OP_DIVIDE
Value op_divide(Value b, Value a);

// OP_MODULO
Value op_modulo(Value b, Value a);

// OP_EQUAL
bool op_equal(Value b, Value a);

// OP_GREATER
Value op_greater(Value b, Value a);

// OP_GREATER_EQUAL
Value op_greater_equal(Value b, Value a);

#endif
