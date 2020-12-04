#ifndef ASPIC_OBJECT_H
#define ASPIC_OBJECT_H

#include "value.h"

typedef enum  {
    OBJECT_STRING
} ObjectType;

struct Object {
    ObjectType type;
    // Each object is a node of the linked list of all allocated objects
    struct Object* next;
};

// First bytes of ObjectString is Object, so an ObjectString* pointer can
// safely be casted to an Object* pointer.
struct ObjectString {
    Object object;
    int length;
    char* chars;
    uint32_t hash;
};

/**
 * ObjectString ctor
 */
ObjectString* string_new(const char* chars, size_t length);
ObjectString* string_concat(const ObjectString* a, const ObjectString* b);
ObjectString* string_multiply(const ObjectString* source, size_t n);

/**
 * Compare two strings for equality
 */
bool string_equal(const ObjectString* a, const ObjectString* b);

/**
 * Compare two strings for ordering
 * @return negative if a < b, zero if a == b, positive if a > b
 */
int string_compare(const ObjectString* a, const ObjectString* b);

/**
 * Object dtor
 */
void object_free(Object* object);

/**
 * Print object to stdout
 */
void object_print(const Object* object);

/**
 * Compare two objects for equality
 */
bool object_equal(const Object* a, const Object* b);

#endif