#include "vm.h"
#include "cfunc.h"
#include "debug.h"
#include "object.h"
#include "parser.h"
#include "shared.h"
#include "utils.h"
#include "value.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Global, unique instance
VM vm;

inline static Value vm_peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

inline static uint8_t vm_read_byte()
{
    // Read next byte and move IP forward by 1
    return *(vm.ip++);
}

inline static uint16_t vm_read_16()
{
    // Read next 2 bytes and move IP forward by 2
    vm.ip += 2;
    return vm.ip[-2] << 8 | vm.ip[-1];
}

inline static Value read_constant()
{
    return vm.chunk->constants.values[*(vm.ip++)];
}

inline static Value read_constant_16()
{
    return vm.chunk->constants.values[vm_read_16()];
}

static void vm_reset_stack()
{
    vm.stack_top = vm.stack;
}

static void vm_push(Value value)
{
    *vm.stack_top = value;
    ++vm.stack_top;
}

static Value vm_pop()
{
#if ASPIC_DEBUG
    if (vm.stack_top == vm.stack) {
        fprintf(stderr, "[fatal] vm_pop: cannot pop empty stack");
        exit(1);
    }
#endif
    return *--vm.stack_top;
}

static void vm_register_fn(const char* name, CFuncPtr fn)
{
    hashtable_set(&vm.globals, string_new(name, strlen(name)), make_cfunc(fn), false);
}

static VmResult vm_report_error(const Value* value)
{
    // -1 because ip points to the next iteration byte
    int offset = (int)(vm.ip - vm.chunk->code - 1);
    int line = chunk_get_line(vm.chunk, offset);
    fprintf(stderr, "RuntimeError at line %d:\n    ", line);
    chunk_print_line(vm.chunk, line);
    fprintf(stderr, "%s\n", value->as.error);

    vm_reset_stack();
    return VM_RUNTIME_ERROR;
}

// Declare a new global variable
static void vm_decl_global(const ObjectString* name, bool read_only)
{
    if (!hashtable_set(&vm.globals, name, vm_pop(), read_only)) {
        vm_push(make_error(formatstr(
            "Identifier '%s' has already been declared", name->chars)));
    }
}

// Push global variable value onto the stack
static void vm_push_global_value(const ObjectString* name)
{
    Value* value = hashtable_get(&vm.globals, name);
    if (value) {
        vm_push(*value);
    } else {
        vm_push(make_error(formatstr(
            "Identifier '%s' is not defined", name->chars)));
    }
}

// Update global variable value
static void vm_update_global_value(const ObjectString* name)
{
    HashtableLookup res = hashtable_update(&vm.globals, name, vm_peek(0));
    if (res == HASHTABLE_MISS) {
        vm_push(make_error(formatstr(
            "Cannot assign to undefined variable '%s'", name->chars)));
    }
    if (res == HASHTABLE_READ_ONLY) {
        vm_push(make_error(formatstr(
            "Cannot assign to constant variable '%s'", name->chars)));
    }
}

static VmResult vm_run()
{
#if ASPIC_DEBUG
    printf("== vm::run ==\n");
#endif
    for (;;) {
#if ASPIC_DEBUG
        instruction_dump(vm.chunk, (int)(vm.ip - vm.chunk->code));
        printf("        [");
        for (const Value* slot = vm.stack; slot < vm.stack_top; ++slot) {
            if (slot != vm.stack) {
                printf(", ");
            }
            value_repr(*slot);
        }
        printf("]\n");
#endif
        // Read next byte
        uint8_t instruction = vm_read_byte();
        switch (instruction) {
        case OP_RETURN:
            return VM_OK;
        case OP_POP:
            vm_pop();
            break;

        // Jumps
        case OP_JUMP:
            vm.ip += vm_read_16();
            break;
        case OP_JUMP_IF_TRUE: {
            uint16_t offset = vm_read_16();
            if (value_truthy(vm_peek(0))) {
                vm.ip += offset;
            }
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = vm_read_16();
            if (!value_truthy(vm_peek(0))) {
                vm.ip += offset;
            }
            break;
        }
        case OP_JUMP_BACK:
            vm.ip -= vm_read_16();
            break;

        // Global variables
        case OP_DECL_GLOBAL:
            vm_decl_global((ObjectString*)read_constant().as.object, false);
            break;
        case OP_DECL_GLOBAL_CONST:
            vm_decl_global((ObjectString*)read_constant().as.object, true);
            break;
        case OP_GET_GLOBAL:
            vm_push_global_value((ObjectString*)read_constant().as.object);
            break;
        case OP_SET_GLOBAL:
            vm_update_global_value((ObjectString*)read_constant().as.object);
            break;
        case OP_DECL_GLOBAL_16:
            vm_decl_global((ObjectString*)read_constant_16().as.object, false);
            break;
        case OP_DECL_GLOBAL_CONST_16:
            vm_decl_global((ObjectString*)read_constant_16().as.object, true);
            break;
        case OP_GET_GLOBAL_16:
            vm_push_global_value((ObjectString*)read_constant_16().as.object);
            break;
        case OP_SET_GLOBAL_16:
            vm_update_global_value((ObjectString*)read_constant_16().as.object);
            break;

        // Local variables
        case OP_GET_LOCAL:
            vm_push(vm.stack[vm_read_byte()]);
            break;
        case OP_SET_LOCAL: {
            uint8_t slot = vm_read_byte();
            vm.stack[slot] = vm_peek(0);
            break;
        }

        // Literals
        case OP_CONSTANT:
            vm_push(read_constant());
            break;
        case OP_CONSTANT_16:
            vm_push(read_constant_16());
            break;

        // Predefined literals
        case OP_ZERO:
            vm_push(make_number(0));
            break;
        case OP_ONE:
            vm_push(make_number(1));
            break;
        case OP_TRUE:
            vm_push(make_bool(true));
            break;
        case OP_FALSE:
            vm_push(make_bool(false));
            break;
        case OP_NULL:
            vm_push(make_null());
            break;

        // Unary operators
        case OP_NOT:
            vm_push(op_not(vm_pop()));
            break;
        case OP_POSITIVE:
            vm_push(op_positive(vm_pop()));
            break;
        case OP_NEGATIVE:
            vm_push(op_negative(vm_pop()));
            break;

        // Binary operators
        case OP_ADD: {
            Value v = vm_pop();
            vm_push(op_add(v, vm_pop()));
            break;
        }
        case OP_SUBTRACT: {
            Value v = vm_pop();
            vm_push(op_subtract(v, vm_pop()));
            break;
        }
        case OP_MULTIPLY: {
            Value v = vm_pop();
            vm_push(op_multiply(v, vm_pop()));
            break;
        }
        case OP_DIVIDE: {
            Value v = vm_pop();
            vm_push(op_divide(v, vm_pop()));
            break;
        }
        case OP_MODULO: {
            Value v = vm_pop();
            vm_push(op_modulo(v, vm_pop()));
            break;
        }

        // Comparators
        case OP_EQUAL:
            vm_push(make_bool(value_equal(vm_pop(), vm_pop())));
            break;
        case OP_NOT_EQUAL:
            vm_push(make_bool(!value_equal(vm_pop(), vm_pop())));
            break;
        case OP_GREATER: {
            Value v = vm_pop();
            vm_push(op_greater(v, vm_pop()));
            break;
        }
        case OP_GREATER_EQUAL: {
            Value v = vm_pop();
            vm_push(op_greater_equal(v, vm_pop()));
            break;
        }
        case OP_LESS: {
            Value v = vm_pop();
            vm_push(op_greater(vm_pop(), v));
            break;
        }
        case OP_LESS_EQUAL: {
            Value v = vm_pop();
            vm_push(op_greater_equal(vm_pop(), v));
            break;
        }

        // Function call
        case OP_CALL: {
            uint8_t argc = vm_read_byte();
            Value fn = vm.stack_top[-(argc + 1)];
            if (fn.type == TYPE_CFUNC) {
                Value result = fn.as.cfunc(vm.stack_top - argc, argc);
                // Pop callee + arguments, then push call result
                vm.stack_top -= (argc + 1);
                vm_push(result);
            } else {
                // The first operand is not a function
                vm_push(make_error(formatstr("Type '%s' is not callable", value_type(fn))));
            }
            break;
        }
        default:
            assert(false); // Unreachable
            break;
        }

        // Check if an error has been pushed in this iteration
        if (vm.stack_top[-1].type == TYPE_ERROR) {
            return vm_report_error(vm.stack_top - 1);
        }
    }
    // Program should end with an empty stack
    assert(vm.stack == vm.stack_top);
    return VM_OK;
}

void vm_init()
{
    vm_reset_stack();
    vm.objects_head = NULL;

    stringset_init(&vm.string_pool);

    // Global variables
    hashtable_init(&vm.globals);

    // Standard functions
    vm_register_fn("assert", aspic_assert);
    vm_register_fn("len", aspic_len);
    vm_register_fn("input", aspic_input);
    vm_register_fn("print", aspic_print);
    vm_register_fn("str", aspic_str);
    vm_register_fn("type", aspic_type);
}

void vm_free()
{
    hashtable_free(&vm.globals);
    stringset_free(&vm.string_pool);

    // Loop on <objects_head> linked list and free every object
    Object* object = vm.objects_head;
    while (object != NULL) {
        Object* next = object->next;
        object_free(object);
        object = next;
    }
}

void vm_register_object(Object* object)
{
    // Preprend object to the linked list
    object->next = vm.objects_head;
    vm.objects_head = object;
}

const ObjectString* vm_find_string(const char* buffer, int length, uint32_t hash)
{
    return stringset_has_cstr(&vm.string_pool, buffer, length, hash);
}

ObjectString* vm_intern_string(ObjectString* string)
{
    // Register in string_pool (set) for string interning
    stringset_add(&vm.string_pool, string);

    // Register in objects_head (linked list) for garbage collecting
    vm_register_object((Object*)string);
    return string;
}

void vm_debug_strings()
{
    printf("=== vm::strings ===\n");
    stringset_print(&vm.string_pool);
}

void vm_debug_globals()
{
    printf("=== vm::globals ===\n");
    hashtable_print(&vm.globals);
}

VmResult vm_interpret(const char* source)
{
    Chunk chunk;
    chunk_init(&chunk, source);

    if (!parser_compile(&chunk)) {
        chunk_free(&chunk);
        return VM_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    // Make instruction pointer (IP) points to the first byte of bytecode
    vm.ip = vm.chunk->code;

    VmResult result = vm_run();
    chunk_free(&chunk);
    return result;
}

Value vm_last_value()
{
    return *vm.stack_top;
}
