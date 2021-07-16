#ifndef ASPIC_VM_H
#define ASPIC_VM_H

#include "chunk.h"
#include "value.h"
#include "hashtable.h"
#include "stringset.h"

#define VM_FRAMES_MAX 64
#define VM_STACK_MAX (VM_FRAMES_MAX * UINT8_MAX)

/**
 * CallFrame represents an ongoing function call
 */
typedef struct {
    // The function being called
    const ObjectFunction* function;
    // Points into the vm.stack at the first slot the function can use
    Value* slots;
    // Instruction pointer: points to the bytecode array (chunk.code)
    uint8_t* ip;
} CallFrame;

typedef struct {
    // Main stack
    Value stack[VM_STACK_MAX];
    Value* stack_top;

    // CallFrame stack (ongoing function calls)
    CallFrame frames[VM_FRAMES_MAX];
    int frame_count;

    // Linked list of allocated objects
    Object* objects_head;

    // Set of all strings
    StringSet string_pool;

    // Hashtable of global variables
    Hashtable globals;

    // Keep a reference to source code for printing lines in stacktrace
    const char* source;
} VM;

typedef enum {
    VM_OK,
    VM_COMPILE_ERROR,
    VM_RUNTIME_ERROR,
} VmResult;

// Ctor
void vm_init();

// Dtor
void vm_free();

/**
 * VM entry point: parse and execute given source code
 * -- source --> [Scanner] -- tokens --> [Parser] -- bytecode --> [VM]
 * @return status code
 */
VmResult vm_interpret(const char* source);

/**
 * Register a dynamically allocated object, to be tracked by the GC
 */
void vm_register_object(Object* object);

/**
 * Check if a string is already interned
 * @return string object if any, otherwise NULL
 */
const ObjectString* vm_find_string(const char* buffer, int length, uint32_t hash);

/**
 * Intern a string object in the VM string pool
 */
ObjectString* vm_intern_string(ObjectString* string);

/**
 * Print all interned strings to stdout
 */
void vm_debug_strings();

/**
 * Print all global variables to stdout
 */
void vm_debug_globals();

/**
 * Get last value pushed to the stack. Useful for REPL.
 */
Value vm_last_value();

#endif
