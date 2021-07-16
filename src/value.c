#include "value.h"
#include "object.h"
#include "shared.h"
#include "utils.h"

#include <stdarg.h> // va_list
#include <stdio.h>
#include <string.h>

Value make_array(const Value* values, int count)
{
    ObjectArray* array = array_new();

    // Allocate memory for 'count' values
    value_array_reserve(&array->array, count);
    for (int i = 0; i < count; ++i) {
        memcpy(array->array.values, values, count * sizeof(Value));
    }
    array->array.count = count;
    return (Value) { .type = TYPE_OBJECT, .as.object = (Object*)array };
}

Value make_number(double value)
{
    return (Value) { .type = TYPE_NUMBER, .as.number = value };
}

Value make_bool(bool value)
{
    return (Value) { .type = TYPE_BOOL, .as.boolean = value };
}

Value make_cfunction(CFuncPtr fn)
{
    return (Value) { .type = TYPE_CFUNC, .as.cfunc = fn };
}

Value make_null()
{
    return (Value) { .type = TYPE_NULL };
}

Value make_error(const char* format, ...)
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

    return (Value) { .type = TYPE_ERROR, .as.error = buffer };
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

// Define max number of printable collection objects (OBJECT_ARRAY).
// Each printed collection has to be tracked to avoid infinite recursion in case
// of circular references.
#define PRINT_MAX_COLLECTIONS 512

/**
 * Recursive printer for values, and collections of values.
 * @param value: value to print
 * @param objects: list of already printed objects, to avoid circular references
 * @param size: size of objects
 * @param depth: recursion depth
 */
static void value_rprinter(Value value, const Object* objects[], int* size, int depth)
{
    switch (value.type) {
    case TYPE_CFUNC:
        printf("<0x%zx()>", (size_t)value.as.cfunc);
        break;

    case TYPE_BOOL:
        printf(value.as.boolean ? "true" : "false");
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
        case OBJECT_ARRAY: {
            const ObjectArray* object = (ObjectArray*)value.as.object;
            // Find if object was already printed
            int i = 0;
            for (; i < *size; ++i) {
                if (objects[i] == (Object*)object) {
                    // This object was already printed, which means some objects
                    // contain themselves. Exit and avoid infinite recursion.
                    printf("[...]");
                    return;
                }
            }
            if (*size < PRINT_MAX_COLLECTIONS) {
                // Track array in case it contains circular references
                objects[(*size)++] = (Object*)object;
                printf("[");
                for (i = 0; i < object->array.count; ++i) {
                    if (i > 0) {
                        printf(", ");
                    }
                    value_rprinter(object->array.values[i], objects, size, depth + 1);
                }
                printf("]");
            } else {
                // Max numbers of collectons reached
                printf("[...]");
            }
            break;
        }

        case OBJECT_FUNCTION: {
            const ObjectFunction* object = (ObjectFunction*)value.as.object;
            if (object->name == NULL) {
                printf("__main__");
            } else {
                printf("<%s()>", object->name->chars);
            }
            break;
        }

        case OBJECT_STRING:
            if (depth == 0) {
                printf("%s", ((const ObjectString*)value.as.object)->chars);
            } else {
                // When nested inside collections, surround strings with quotes
                printf("\"%s\"", ((const ObjectString*)value.as.object)->chars);
            }
            break;
        }

        break;
    }
}

void value_print(Value value)
{
    const Object* objects[PRINT_MAX_COLLECTIONS] = { NULL };
    int size = 0;
    value_rprinter(value, objects, &size, 0);
}

void value_repr(Value value)
{
    const Object* objects[PRINT_MAX_COLLECTIONS] = { NULL };
    int size = 0;
    // The only difference between 'print' and 'repr' is that 'print' will not
    // surround top-level strings with quotes.
    //     print("hello") => hello
    //     repr("hello") => "hello"
    // Trick the printer by starting with a depth of 1, so value will
    // be surrounded with quotes if it's a string.
    value_rprinter(value, objects, &size, 1);
}

const char* value_type(Value value)
{
    switch (value.type) {
    case TYPE_BOOL:
        return "bool";
    case TYPE_CFUNC:
        return "cfunction";
    case TYPE_ERROR:
        return "error";
    case TYPE_NULL:
        return "null";
    case TYPE_NUMBER:
        return "number";
    case TYPE_OBJECT:
        switch (value.as.object->type) {
        case OBJECT_ARRAY:
            return "array";
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
