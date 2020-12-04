#ifndef ASPIC_VALUE_ARRAY_H
#define ASPIC_VALUE_ARRAY_H

#include "value.h"

/**
 * A dynamic array for Value items
 */
typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

// Ctor
void value_array_init(ValueArray* value_array);

// Dtor
void value_array_free(ValueArray* value_array);

// Append a value
void value_array_push(ValueArray* value_array, Value value);

#endif
