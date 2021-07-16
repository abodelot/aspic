#include "op_code.h"
#include "object.h"
#include "utils.h"

#include <stdlib.h>

#define STROP(x) \
    case x:      \
        return #x;

const char* op2str(OpCode opcode)
{
    switch (opcode) {
        STROP(OP_RETURN)
        STROP(OP_POP)
        STROP(OP_JUMP)
        STROP(OP_JUMP_IF_TRUE)
        STROP(OP_JUMP_IF_FALSE)
        STROP(OP_JUMP_BACK)
        STROP(OP_DECL_GLOBAL)
        STROP(OP_DECL_GLOBAL_CONST)
        STROP(OP_GET_GLOBAL)
        STROP(OP_SET_GLOBAL)
        STROP(OP_DECL_GLOBAL_16)
        STROP(OP_DECL_GLOBAL_CONST_16)
        STROP(OP_GET_GLOBAL_16)
        STROP(OP_SET_GLOBAL_16)
        STROP(OP_GET_LOCAL)
        STROP(OP_SET_LOCAL)
        STROP(OP_CONSTANT)
        STROP(OP_CONSTANT_16)
        STROP(OP_ZERO)
        STROP(OP_ONE)
        STROP(OP_TRUE)
        STROP(OP_FALSE)
        STROP(OP_NULL)
        STROP(OP_NOT)
        STROP(OP_POSITIVE)
        STROP(OP_NEGATIVE)
        STROP(OP_ADD)
        STROP(OP_SUBTRACT)
        STROP(OP_MULTIPLY)
        STROP(OP_DIVIDE)
        STROP(OP_MODULO)
        STROP(OP_EQUAL)
        STROP(OP_NOT_EQUAL)
        STROP(OP_GREATER)
        STROP(OP_GREATER_EQUAL)
        STROP(OP_LESS)
        STROP(OP_LESS_EQUAL)
        STROP(OP_SUBSCRIPT_GET)
        STROP(OP_SUBSCRIPT_SET)
        STROP(OP_CALL)
        STROP(OP_ARRAY)
    }
    return NULL;
}

static Value binary_op_error(OpCode op, Value a, Value b)
{
    return make_error("Unsupported operator %s for types <%s> and <%s>",
        op2str(op),
        value_type(a),
        value_type(b));
}

static Value unary_op_error(OpCode op, Value a)
{
    return make_error("Unsupported operator %s for type <%s>",
        op2str(op),
        value_type(a));
}

static Value comparison_error(Value a, Value b)
{
    return make_error("Cannot compare types <%s> and <%s>",
        value_type(a),
        value_type(b));
}

Value op_not(Value value)
{
    switch (value.type) {
    case TYPE_BOOL:
        return make_bool(!value.as.boolean);
    case TYPE_NULL:
        return make_bool(true);
    default:
        return make_bool(false);
    }
}

Value op_positive(Value value)
{
    switch (value.type) {
    case TYPE_NUMBER:
        return value; // NOOP
    default:
        return unary_op_error(OP_POSITIVE, value);
    }
}

Value op_negative(Value value)
{
    switch (value.type) {
    case TYPE_NUMBER:
        return make_number(-value.as.number);
    default:
        return unary_op_error(OP_NEGATIVE, value);
    }
}

Value op_add(Value b, Value a)
{
    if (a.type == TYPE_NUMBER && b.type == TYPE_NUMBER) {
        return make_number(a.as.number + b.as.number);
    }

    if (a.type == TYPE_OBJECT
        && b.type == TYPE_OBJECT
        && a.as.object->type == OBJECT_STRING
        && b.as.object->type == OBJECT_STRING) {
        return make_string(
            string_concat((const ObjectString*)a.as.object, (const ObjectString*)b.as.object));
    }
    return binary_op_error(OP_ADD, a, b);
}

Value op_subtract(Value b, Value a)
{
    if (a.type == TYPE_NUMBER && b.type == TYPE_NUMBER) {
        return make_number(a.as.number - b.as.number);
    }
    return binary_op_error(OP_SUBTRACT, a, b);
}

Value op_multiply(Value b, Value a)
{
    // <number> * <number>
    if (a.type == TYPE_NUMBER && b.type == TYPE_NUMBER) {
        return make_number(a.as.number * b.as.number);
    }

    // <string> * <number>
    if (a.type == TYPE_OBJECT && b.type == TYPE_NUMBER && a.as.object->type == OBJECT_STRING) {
        return make_string(
            string_multiply((const ObjectString*)a.as.object, b.as.number));
    }

    // <number> * <string>
    if (a.type == TYPE_NUMBER && b.type == TYPE_OBJECT && b.as.object->type == OBJECT_STRING) {
        return make_string(
            string_multiply((const ObjectString*)b.as.object, a.as.number));
    }

    return binary_op_error(OP_MULTIPLY, a, b);
}

Value op_divide(Value b, Value a)
{
    if (a.type == TYPE_NUMBER && b.type == TYPE_NUMBER) {
        if (b.as.number == 0) {
            return make_error("Cannot divide by 0");
        }
        return make_number(a.as.number / b.as.number);
    }
    return binary_op_error(OP_DIVIDE, a, b);
}

Value op_modulo(Value b, Value a)
{
    if (a.type == TYPE_NUMBER && b.type == TYPE_NUMBER) {
        return make_number((int)a.as.number % (int)b.as.number);
    }
    return binary_op_error(OP_MODULO, a, b);
}

Value op_greater(Value b, Value a)
{
    if (a.type == b.type) {
        switch (a.type) {
        case TYPE_NUMBER:
            return make_bool(a.as.number > b.as.number);
        case TYPE_OBJECT:
            if (a.as.object->type == OBJECT_STRING && b.as.object->type == OBJECT_STRING) {
                return make_bool(
                    string_compare(
                        (const ObjectString*)a.as.object, (const ObjectString*)b.as.object)
                    > 0);
            }
        default:
            break;
        }
    }
    return comparison_error(a, b);
}

Value op_greater_equal(Value b, Value a)
{
    if (a.type == b.type) {
        switch (a.type) {
        case TYPE_NUMBER:
            return make_bool(a.as.number >= b.as.number);
        case TYPE_OBJECT:
            if (a.as.object->type == OBJECT_STRING && b.as.object->type == OBJECT_STRING) {
                return make_bool(
                    string_compare(
                        (const ObjectString*)a.as.object, (const ObjectString*)b.as.object)
                    >= 0);
            }
        default:
            break;
        }
    }
    return comparison_error(a, b);
}

Value op_subscript_get(Value collection, Value index)
{
    if (collection.type == TYPE_OBJECT && index.type == TYPE_NUMBER) {
        int i = (int)index.as.number;
        if (collection.as.object->type == OBJECT_STRING) {
            const ObjectString* string = (const ObjectString*)collection.as.object;
            if (i >= -string->length && i < string->length) {
                if (i < 0) {
                    i += string->length;
                }
                return make_string_from_buffer(string->chars + i, 1);
            }
            // Index out of range
            return make_error("string index %d is out of range [%d:%d]",
                i, -string->length, string->length - 1);
        }

        if (collection.as.object->type == OBJECT_ARRAY) {
            const ObjectArray* object = (const ObjectArray*)collection.as.object;
            if (i >= -object->array.count && i < object->array.count) {
                if (i < 0) {
                    i += object->array.count;
                }
                return object->array.values[i];
            }
            // Index out of range
            return make_error("array index %d is out of range [%d:%d]",
                i, -object->array.count, object->array.count - 1);
        }
    }
    return binary_op_error(OP_SUBSCRIPT_GET, collection, index);
}

Value op_subscript_set(Value collection, Value index, Value value)
{
    if (collection.type == TYPE_OBJECT && collection.as.object->type == OBJECT_ARRAY) {
        if (index.type != TYPE_NUMBER) {
            return make_error("index must be an integer, not '%s'", value_type(index));
        }

        ObjectArray* object = (ObjectArray*)collection.as.object;
        int i = (int)index.as.number;
        if (i >= -object->array.count && i < object->array.count) {
            if (i < 0) {
                i += object->array.count;
            }
            object->array.values[i] = value;
            return value;
        }
        // Index out of range
        return make_error("array index %d is out of range [%d:%d]",
            i, -object->array.count, object->array.count - 1);
    }
    return make_error("'%s' does not support item assignment", value_type(collection));
}
