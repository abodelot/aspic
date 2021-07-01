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
void value_array_init(ValueArray* self);

// Dtor
void value_array_free(ValueArray* self);

// Append a value
void value_array_push(ValueArray* self, Value value);

/**
 * Find a given value
 * @return value index in array if present, otherwise -1
 */
int value_array_find(const ValueArray* self, Value value);

#endif
