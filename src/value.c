#include "value.h"
#include "object.h"
#include "shared.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

Value make_number(double value)
{
    return (Value) { .type = TYPE_NUMBER, .as.number = value };
}

Value make_bool(bool value)
{
    return (Value) { .type = TYPE_BOOL, .as.boolean = value };
}

Value make_cfunc(CFuncPtr fn)
{
    return (Value) { .type = TYPE_CFUNC, .as.cfunc = fn };
}

Value make_null()
{
    return (Value) { .type = TYPE_NULL };
}

Value make_error(const char* error)
{
    return (Value) { .type = TYPE_ERROR, .as.error = error };
}

Value make_string(const ObjectString* string)
{
    return (Value) { .type = TYPE_OBJECT, .as.object = (Object*)string };
}

Value make_string_from_buffer(const char* chars, int length)
{
    return (Value) {
        .type = TYPE_OBJECT,
        .as.object = (Object*)string_new(chars, length)
    };
}

Value make_string_from_cstr(const char* str)
{
    return (Value) {
        .type = TYPE_OBJECT,
        .as.object = (Object*)string_new(str, strlen(str))
    };
}

Value make_function(ObjectFunction* fn)
{
    return (Value) { .type = TYPE_OBJECT, .as.object = (Object*)fn };
}

void value_repr(Value value)
{
    switch (value.type) {
    case TYPE_BOOL:
        printf(value.as.boolean ? "true" : "false");
        break;
    case TYPE_CFUNC:
        printf("<cfunc @%lx>", (size_t)value.as.cfunc);
        break;
    case TYPE_NUMBER:
        printf("%g", value.as.number);
        break;
    case TYPE_NULL:
        printf("null");
        break;
    case TYPE_ERROR:
        printf("[RuntimeError] %s", value.as.error);
        break;
    case TYPE_OBJECT:
        switch (value.as.object->type) {
        case OBJECT_FUNCTION: {
            const ObjectString* name = ((ObjectFunction*)value.as.object)->name;
            if (name != NULL) {
                printf("<function %s>", name->chars);
            } else {
                printf("__main__");
            }
            break;
        }
        case OBJECT_STRING:
            printf("\"%s\"", ((ObjectString*)value.as.object)->chars);
            break;
        }
        break;
    }
}

const char* value_type(Value value)
{
    switch (value.type) {
    case TYPE_BOOL:
        return "bool";
    case TYPE_CFUNC:
        return "cfunc";
    case TYPE_ERROR:
        return "error";
    case TYPE_NULL:
        return "null";
    case TYPE_NUMBER:
        return "number";
    case TYPE_OBJECT:
        switch (value.as.object->type) {
        case OBJECT_FUNCTION:
            return "function";
        case OBJECT_STRING:
            return "string";
        }
    }
    return NULL;
}

bool value_equal(Value b, Value a)
{
    if (a.type == b.type) {
        switch (a.type) {
        case TYPE_BOOL:
            return a.as.boolean == b.as.boolean;
        case TYPE_NULL:
            // Two null values are always equal
            return true;
        case TYPE_NUMBER:
            return a.as.number == b.as.number;
        case TYPE_OBJECT:
            return object_equal(a.as.object, b.as.object);
        default:
            break; // Unreachable
        }
    }
    return false;
}

bool value_truthy(Value value)
{
    // Only false and null are false, everything else is truthy
    return !(value.type == TYPE_NULL || (value.type == TYPE_BOOL && value.as.boolean == false));
}
