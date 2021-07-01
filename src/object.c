#include "object.h"
#include "vm.h"

#include <stdio.h>
#include <string.h>

static uint32_t hash_string(const char* str, size_t length)
{
    // FNV-1a hash function
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < length; ++i) {
        hash ^= str[i];
        hash *= 16777619;
    }

    return hash;
}

// Allocate an empty ObjectString with requested length
static ObjectString* string_allocate(size_t length)
{
    ObjectString* string = malloc(sizeof(ObjectString));
    if (string == NULL) {
        fprintf(stderr, "Cannot allocate String object.\n");
        exit(1);
    }

    // Shared object attributes
    string->object.type = OBJECT_STRING;
    string->object.next = NULL;

    // String attributes
    string->chars = malloc(length + 1);
    if (string->chars == NULL) {
        fprintf(stderr, "Cannot allocate string of length %ld\n", length);
        exit(1);
    }

    string->chars[length] = '\0';
    string->length = length;

    return string;
}

// ctor: handle string interning
const ObjectString* string_new(const char* chars, size_t length)
{
    uint32_t hash = hash_string(chars, length);

    // Check if string is already interned in the VM
    const ObjectString* interned = vm_find_string(chars, length, hash);
    if (interned) {
        return interned;
    }

    ObjectString* string = string_allocate(length);
    memcpy(string->chars, chars, length);

    string->hash = hash;

    return vm_intern_string(string);
}

// ctor: handle string interning
const ObjectString* string_concat(const ObjectString* a, const ObjectString* b)
{
    size_t length = a->length + b->length;
    ObjectString* string = string_allocate(length);

    memcpy(string->chars, a->chars, a->length);
    memcpy(string->chars + a->length, b->chars, b->length);

    string->hash = hash_string(string->chars, length);

    // Check if string was already interned in the VM
    const ObjectString* interned = vm_find_string(string->chars, length, string->hash);
    if (interned) {
        // Destroy object and return interned instead
        object_free((Object*)string);
        return interned;
    }

    return vm_intern_string(string);
}

// ctor: handle string interning
const ObjectString* string_multiply(const ObjectString* source, size_t n)
{
    size_t length = source->length * n;
    ObjectString* string = string_allocate(length);

    for (size_t i = 0; i < n; ++i) {
        memcpy(string->chars + i * source->length, source->chars, source->length);
    }

    string->hash = hash_string(string->chars, length);

    // Check if string was already interned in the VM
    const ObjectString* interned = vm_find_string(string->chars, length, string->hash);
    if (interned) {
        // Destroy object and return interned instead
        object_free((Object*)string);
        return interned;
    }

    return vm_intern_string(string);
}

bool string_equal(const ObjectString* a, const ObjectString* b)
{
    // Because all strings are deduplicated and interned (vm.string_pool),
    // simply compare pointers
    return a == b;
}

int string_compare(const ObjectString* a, const ObjectString* b)
{
    return strcmp(a->chars, b->chars);
}

void object_free(Object* object)
{
    switch (object->type) {
    case OBJECT_STRING: {
        ObjectString* string = (ObjectString*)object;
        // Destroy the allocated string buffer
        free(string->chars);
        string->chars = NULL;
        // Destroy the object itself
        free(string);
        break;
    }
    }
}

void object_print(const Object* object)
{
    switch (object->type) {
    case OBJECT_STRING:
        printf("%s", ((const ObjectString*)object)->chars);
        break;
    }
}

bool object_equal(const Object* a, const Object* b)
{
    if (a->type == b->type) {
        if (a->type == OBJECT_STRING) {
            return string_equal((const ObjectString*)a, (const ObjectString*)b);
        }
    }
    return false;
}
