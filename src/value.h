#ifndef ASPIC_VALUE_H
#define ASPIC_VALUE_H

#include "shared.h"

typedef struct Object Object;
typedef struct ObjectString ObjectString;
typedef struct ObjectFunction ObjectFunction;
typedef struct ObjectArray ObjectArray;

typedef enum {
    TYPE_CFUNC, // native C function
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NULL,
    TYPE_ERROR,
    TYPE_OBJECT,
} ValueType;

typedef struct Value (*CFuncPtr)(struct Value* argv, int args);

typedef union {
    double number;     // TYPE_CFUNC
    bool boolean;      // TYPE_BOOL
    const char* error; // TYPE_ERROR
    Object* object;    // TYPE_OBJECT
    CFuncPtr cfunc;    // TYPE_CFUNC
} ValueData;

typedef struct Value {
    ValueData as;
    ValueType type;
} Value;

Value make_array(const Value* values, int count);

/**
 * Ctors for Value
 */
Value make_bool(bool value);

Value make_cfunction(CFuncPtr fn);

/**
 * Build an error (TYPE_ERROR)
 * Error message is dynamically allocated
 */
Value make_error(const char* format, ...);

Value make_function(ObjectFunction* fn);

Value make_null();

Value make_number(double value);

// Build a String, from an already created ObjectString
Value make_string(const ObjectString* string);

// Build a String, from a buffer of chars, will create an internal ObjectString
Value make_string_from_buffer(const char* chars, int length);

// Build a String, from a buffer ending with '\0', will create an internal ObjectString
Value make_string_from_cstr(const char* str);

/**
 * Print value to stdout
 */
void value_print(Value value);

/**
 * Print value to stdout in their canonical representation
 */
void value_repr(Value value);

/**
 * Get a string representation of the value's type
 */
const char* value_type(Value value);

/**
 * Check if two values are equal
 */
bool value_equal(Value b, Value a);

/**
 * Convert a value to boolean
 */
bool value_truthy(Value value);

#endif
