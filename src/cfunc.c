#include "cfunc.h"
#include "object.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

Value aspic_assert(Value* argv, int argc)
{
    if (argc != 1) {
        return make_error(formatstr("assert() expects 1 argument, got %d", argc));
    }

    if (!value_truthy(argv[0])) {
        return make_error("Assertion failed");
    }
    return make_bool(true);
}

Value aspic_input(Value* argv, int argc)
{
    if (argc > 1) {
        return make_error(formatstr("input() expects 1 argument at most, got %d", argc));
    }

    // Configure readline to insert tabs (instead of PATH completion)
    rl_bind_key('\t', rl_insert);

    // If 1 string argument was provided, display it as prompt
    const char* prompt = argc == 1
        ? ((ObjectString*)aspic_str(argv, argc).as.object)->chars
        : NULL;
    char* line = readline(prompt);
    if (line != NULL) {
        add_history(line);
        Value ret = make_string_from_cstr(line);
        free(line);
        return ret;
    }
    return make_null();
}

Value aspic_len(Value* argv, int argc)
{
    if (argc != 1) {
        return make_error(formatstr("len() expects 1 argument, got %d", argc));
    }

    if (argv[0].type == TYPE_OBJECT && argv[0].as.object->type == OBJECT_STRING) {
        return make_number(
            ((const ObjectString*)argv[0].as.object)->length);
    }
    return make_error(formatstr("Cannot get length for type %s", value_type(*argv)));
}

Value aspic_print(Value* argv, int argc)
{
    for (int i = 0; i < argc; ++i) {
        if (i > 0) {
            putchar(' ');
        }
        ObjectString* str = (ObjectString*)aspic_str(argv + i, 1).as.object;
        printf("%s", str->chars);
    }
    putchar('\n');
    return make_null();
}

Value aspic_str(Value* argv, int argc)
{
    if (argc != 1) {
        return make_error(formatstr("str() expects 1 argument, got %d", argc));
    }

    switch (argv[0].type) {
    case TYPE_CFUNC: {
        char buffer[64];
        snprintf(buffer, sizeof buffer, "<cfunc %lx>", (size_t)argv[0].as.cfunc);
        return make_string_from_cstr(buffer);
    }
    case TYPE_BOOL: {
        const char* cstr = argv[0].as.boolean ? "true" : "false";
        return make_string_from_cstr(cstr);
    }
    case TYPE_NUMBER: {
        char buffer[64];
        snprintf(buffer, sizeof buffer, "%g", argv[0].as.number);
        return make_string_from_cstr(buffer);
    }
    case TYPE_NULL:
        return make_string_from_cstr("");
    case TYPE_OBJECT:
        switch (argv[0].as.object->type) {
        case OBJECT_FUNCTION:
            return make_string(((ObjectFunction*)argv[0].as.object)->name);
        case OBJECT_STRING:
            return argv[0];
        }
        break;
    default:
        break;
    }
    return make_null();
}

Value aspic_type(Value* argv, int argc)
{
    if (argc != 1) {
        make_error(formatstr("type() expects 1 argument, got %d", argc));
    }

    return make_string_from_cstr(value_type(*argv));
}
