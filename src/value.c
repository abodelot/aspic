#include "value.h"
#include "object.h"
#include "shared.h"

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

Value make_string(ObjectString* string)
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

void value_print(Value value)
{
    switch (value.type) {
    case TYPE_BOOL:
        printf(value.as.boolean ? "true" : "false");
        break;
    case TYPE_CFUNC:
        printf("cfunc <%p>", (void*)value.as.cfunc);
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
        object_print(value.as.object);
        break;
    }
}

void value_repr(Value value)
{
    switch (value.type) {
    case TYPE_NUMBER:
        printf("%g", value.as.number);
        break;
    case TYPE_OBJECT:
        if (value.as.object->type == OBJECT_STRING) {
            printf("\"%s\"", ((ObjectString*)value.as.object)->chars);
        }
        break;
    default:
        value_print(value);
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
        case OBJECT_STRING:
            return "string";
        }
    }
    return NULL;
}

bool value_truthy(Value value)
{
    // Only false and null are false, everything else is truthy
    return !(value.type == TYPE_NULL || (value.type == TYPE_BOOL && value.as.boolean == false));
}
