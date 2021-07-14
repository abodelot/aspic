#include "parser.h"
#include "chunk.h"
#include "debug.h"
#include "op_code.h"
#include "scanner.h"
#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    Token current;
    Token previous;
    bool errored;
    bool panic_mode;
    const char* source;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // ||
    PREC_AND,        // &&
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * / %
    PREC_UNARY,      // ! -
    PREC_CALL,       // . () []
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static void rule_grouping(bool);
static void rule_unary_op(bool);
static void rule_binary_op(bool);
static void rule_number(bool);
static void rule_string(bool);
static void rule_literal(bool);
static void rule_fn_call(bool);
static void rule_subscript(bool);
static void rule_variable(bool);
static void rule_and(bool);
static void rule_or(bool);

ParseRule rules[] = {
    // IMPORTANT: keep synced with TokenType enum
    // Single-character tokens
    [TOKEN_LEFT_PAREN] = { rule_grouping, rule_fn_call, PREC_CALL },
    [TOKEN_RIGHT_PAREN] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACKET] = { NULL, rule_subscript, PREC_CALL },
    [TOKEN_RIGHT_BRACKET] = { NULL, NULL, PREC_NONE },
    [TOKEN_COMMA] = { NULL, NULL, PREC_NONE },
    [TOKEN_DOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_MINUS] = { rule_unary_op, rule_binary_op, PREC_TERM },
    [TOKEN_PERCENT] = { NULL, rule_binary_op, PREC_FACTOR },
    [TOKEN_PLUS] = { rule_unary_op, rule_binary_op, PREC_TERM },
    [TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE },
    [TOKEN_SLASH] = { NULL, rule_binary_op, PREC_FACTOR },
    // One or more character tokens
    [TOKEN_AMPER] = { NULL, NULL, PREC_NONE },
    [TOKEN_AMPER_AMPER] = { NULL, rule_and, PREC_AND },
    [TOKEN_PIPE] = { NULL, NULL, PREC_NONE },
    [TOKEN_PIPE_PIPE] = { NULL, rule_or, PREC_OR },
    [TOKEN_STAR] = { NULL, rule_binary_op, PREC_FACTOR },
    [TOKEN_BANG] = { rule_unary_op, NULL, PREC_NONE },
    [TOKEN_BANG_EQUAL] = { NULL, rule_binary_op, PREC_EQUALITY },
    [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL_EQUAL] = { NULL, rule_binary_op, PREC_EQUALITY },
    [TOKEN_GREATER] = { NULL, rule_binary_op, PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL] = { NULL, rule_binary_op, PREC_COMPARISON },
    [TOKEN_LESS] = { NULL, rule_binary_op, PREC_COMPARISON },
    [TOKEN_LESS_EQUAL] = { NULL, rule_binary_op, PREC_COMPARISON },
    // Literals
    [TOKEN_IDENTIFIER] = { rule_variable, NULL, PREC_NONE },
    [TOKEN_STRING] = { rule_string, NULL, PREC_NONE },
    [TOKEN_NUMBER] = { rule_number, NULL, PREC_NONE },
    // Keywords
    [TOKEN_CLASS] = { NULL, NULL, PREC_NONE },
    [TOKEN_CONST] = { NULL, NULL, PREC_NONE },
    [TOKEN_DEF] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_FALSE] = { rule_literal, NULL, PREC_NONE },
    [TOKEN_IF] = { NULL, NULL, PREC_NONE },
    [TOKEN_LET] = { NULL, NULL, PREC_NONE },
    [TOKEN_NULL] = { rule_literal, NULL, PREC_NONE },
    [TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
    [TOKEN_SUPER] = { NULL, NULL, PREC_NONE },
    [TOKEN_THIS] = { NULL, NULL, PREC_NONE },
    [TOKEN_TRUE] = { rule_literal, NULL, PREC_NONE },
    [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
    [TOKEN_ERROR] = { NULL, NULL, PREC_NONE },
    [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
};

/*
 * Global variables are resolved at runtime. The identifiers and their values
 * are stored in the vm.globals hashmap.
 * The parser emits OP_*_GLOBAL_* instructions, used by the VM to
 * create/update/read globals.
 * Those instructions take an operand, which is the index of the Value in the
 * chunk.constants array, representing the global's name as a string.
 * This string value is matched at runtime with keys in the vm.globals hashmap.
 *
 * Local variables are resolved at compile time, which means there isn't any
 * instruction to declare a local. The values are stored directly on the VM stack.
 * Instructions OP_GET_LOCAL/OP_SET_LOCAL take an operand, which is the local
 * index in the VM stack. As a consequence, resolving locals is much faster than
 * resolving globals, but the VM isn't able to keep track of the locals original
 * names at runtime.
 *
 * Locals and globals use a different mechanism because globals can be refered
 * before the parser encounters their declaration.
 */

typedef struct {
    Token name; // name of the variable
    int depth;  // scope depth of the variable
    bool read_only;
} Local;

typedef enum {
    CHUNK_FUNCTION,
    CHUNK_MAIN
} ChunkType;

typedef struct Compiler {
    // Keep track of the previous current compiler instance
    struct Compiler* previous;

    ObjectFunction* function;
    ChunkType type;

    Local locals[UINT8_MAX];
    int local_count;
    int scope_depth;
} Compiler;

Parser parser;
Compiler* g_compiler = NULL;

static Chunk* current_chunk()
{
    return &g_compiler->function->chunk;
}

static void error_at(const Token* token, const char* message)
{
    // If the panic_mode flag is already set, suppress any other errors
    // that get detected. Otherwise, turn on panic_mode flag.
    if (parser.panic_mode) {
        return;
    }
    parser.panic_mode = true;
    fprintf(stderr, "SyntaxError at line %d:\n    ", token->line);
    print_line(stderr, parser.source, token->line);

    fprintf(stderr, "%s", message);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at EOF\n");
    } else if (token->type == TOKEN_ERROR) {
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, " at '%.*s'\n", token->length, token->start);
    }

    parser.errored = true;
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}

static void error_at_current(const char* message)
{
    error_at(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = next_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        // Report error from the scanner
        error_at_current(parser.current.start);
    }
}

/**
 * Read the next token from the scanner, an trigger and error if next token
 * has a different type than the expected one.
 * @param type: the expected type for next token
 * @param message: error message to report
 */
static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

static bool match(TokenType type)
{
    // Consumme current token if it matches the given type
    if (parser.current.type != type) {
        return false;
    }
    advance();
    return true;
}

static void parse_precedence(Precedence precedence)
{
    advance();
    // Rule for unary operators
    ParseFn prefix_rule = rules[parser.previous.type].prefix;
    if (prefix_rule == NULL) {
        error("Expected expression");
        return;
    }

    bool assignable = precedence <= PREC_ASSIGNMENT;
    prefix_rule(assignable);

    // Rule for binary operators
    while (precedence <= rules[parser.current.type].precedence) {
        advance();
        ParseFn infix_rule = rules[parser.previous.type].infix;
        infix_rule(assignable);
    }

    if (assignable && match(TOKEN_EQUAL)) {
        error("Invalid left-hand side in assignment");
    }
}

/**
 * Append a single byte of instruction to current chunk
 */
static void emit_byte(uint8_t byte)
{
    chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_return()
{
    if (g_compiler->type != CHUNK_MAIN) {
        // Implicit NULL return value for functions
        emit_byte(OP_NULL);
    }
    emit_byte(OP_RETURN);
}

static int emit_constant(Value constant)
{
    // Register the constant value in the chunk
    Chunk* chunk = current_chunk();
    int constant_index = chunk_register_constant(chunk, constant);

    // Write load instruction + index
    if (!chunk_write_constant(current_chunk(), constant_index, parser.previous.line)) {
        error("Too many constants in one chunk");
    }
    return constant_index;
}

static ObjectFunction* end_compiler()
{
    emit_return();
    ObjectFunction* function = g_compiler->function;
#ifdef ASPIC_DEBUG
    if (!parser.errored) {
        chunk_dump(current_chunk(), function->name ? function->name->chars : "__main__");
    }
#endif
    // Restore the previous instance as the current one
    g_compiler = g_compiler->previous;
    return function;
}

static void begin_scope()
{
    g_compiler->scope_depth++;
}

static void end_scope()
{
    g_compiler->scope_depth--;

    // When exiting a scope, loop backward through the local array looking for
    // any variables declared at the scope depth we just left.
    // Discard them by simply decrementing the length of the local array.
    while (g_compiler->local_count > 0 && g_compiler->locals[g_compiler->local_count - 1].depth > g_compiler->scope_depth) {
        emit_byte(OP_POP);
        g_compiler->local_count--;
    }
}

static void synchronize()
{
    parser.panic_mode = false;

    // Skip tokens until we reach something that looks like the beginning
    // of a statement
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
        case TOKEN_CLASS:
        case TOKEN_CONST:
        case TOKEN_DEF:
        case TOKEN_IF:
        case TOKEN_LET:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
            return;
        default:
            break;
        }
        advance();
    }
}

static void expression()
{
    // Start with parse the lowest precedence level
    parse_precedence(PREC_ASSIGNMENT);
}

static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression");
    // Discard the expression result
    emit_byte(OP_POP);
}

static void add_local_variable(const Token* name)
{
    if (g_compiler->local_count == UINT8_MAX) {
        error("Too many local variables in function");
        return;
    }

    Local* local = &g_compiler->locals[g_compiler->local_count++];
    local->name = *name;
    local->depth = -1;       // Flag variable as not initialized yet
    local->read_only = true; // Doesn't matter, not initialized yet
}

static bool identifiers_equal(const Token* a, const Token* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static int parse_variable(const char* error_message)
{
    consume(TOKEN_IDENTIFIER, error_message);

    // If in a scope, it's a local variable
    if (g_compiler->scope_depth > 0) {
        const Token* name = &parser.previous;

        // Detect if variable was already declared
        for (int i = g_compiler->local_count - 1; i >= 0; i--) {
            const Local* local = &g_compiler->locals[i];
            if (local->depth != -1 && local->depth < g_compiler->scope_depth) {
                break;
            }

            if (identifiers_equal(name, &local->name)) {
                error("Already variable with this name in this scope");
            }
        }
        add_local_variable(name);
        // When in a local scope, no need to store the variable name in the
        // chunk.constants array. Return a dummy index instead.
        return -1;
    }

    // Register the identifer as a constant value in the chunk
    Value identifier = make_string_from_buffer(parser.previous.start, parser.previous.length);
    Chunk* chunk = current_chunk();
    return chunk_register_constant(chunk, identifier);
}

static void declare_global(int global_index, bool read_only)
{
    // Declare global variable
    if (global_index <= UINT8_MAX) {
        emit_byte(read_only ? OP_DECL_GLOBAL_CONST : OP_DECL_GLOBAL);
        emit_byte(global_index);
    } else if (global_index <= UINT16_MAX) {
        emit_byte(read_only ? OP_DECL_GLOBAL_CONST_16 : OP_DECL_GLOBAL_16);
        // Convert global index to a 2-bytes integer
        emit_byte((global_index >> 8) & 0xff);
        emit_byte(global_index & 0xff);
    } else {
        error("Cannot declare over UINT16_MAX constants");
    }
}

static void var_declaration(bool read_only)
{
    int global_index = parse_variable("Expected variable name");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NULL);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");

    if (g_compiler->scope_depth > 0) {
        assert(global_index == -1);
        // Initialize local variable
        Local* last = &g_compiler->locals[g_compiler->local_count - 1];
        last->depth = g_compiler->scope_depth;
        last->read_only = read_only;
    } else {
        assert(global_index >= 0);
        declare_global(global_index, read_only);
    }
}

static void compiler_init(Compiler* compiler, ChunkType type)
{
    compiler->previous = g_compiler;
    compiler->function = NULL;
    compiler->type = type;

    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function = function_new();
    g_compiler = compiler;

    // compiler_init is called right after parsing the function name
    // Extract the name from the previous token
    if (type != CHUNK_MAIN) {
        compiler->function->name = string_new(parser.previous.start,
            parser.previous.length);
    }

    // Claim an empty Local slot with empty name for internal use
    // Users cannot declare locals named "" so it won't collide
    Local* local = &g_compiler->locals[g_compiler->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static void statement();
static void block();

/**
 * Parse the function arguments and body
 */
static void parse_function(ChunkType type)
{
    Compiler compiler;
    compiler_init(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PAREN, "Expected '(' after function name");

    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        do {
            g_compiler->function->arity++;
            // Arity of OP_CALL is stored on a single byte
            if (g_compiler->function->arity > UINT8_MAX) {
                error_at_current("Function cannot have more than 255 parameters.");
            }
            int constant = parse_variable("Expected parameter name");
            // Function arguments are local variables
            assert(constant == -1);
            assert(g_compiler->scope_depth > 0);

            // Mark initialized
            Local* last = &g_compiler->locals[g_compiler->local_count - 1];
            last->depth = g_compiler->scope_depth;

            // Function arguments can be reassigned
            last->read_only = false;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before function body");
    block();

    ObjectFunction* function = end_compiler();
    emit_constant(make_function(function));
}

static void function_declaration()
{
    // Function declaration follows the same logic than variables: globals
    // when at top-level, locals when inside a scope.
    uint8_t global = parse_variable("Expected function name");

    // If local
    if (g_compiler->scope_depth > 0) {
        // Mark as initialized
        Local* last = &g_compiler->locals[g_compiler->local_count - 1];
        last->depth = g_compiler->scope_depth;
        // Allow function name to be rebinded
        last->read_only = false;
    }
    parse_function(CHUNK_FUNCTION);

    if (g_compiler->scope_depth == 0) {
        declare_global(global, false);
    }
}

static void patch_jump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    uint8_t* code = current_chunk()->code;
    code[offset] = (jump >> 8) & 0xff;
    code[offset + 1] = jump & 0xff;
}

static int emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    // Write a 2-bytes placeholder operand for the jump
    emit_byte(0xff);
    emit_byte(0xff);
    return current_chunk()->count - 2;
}

static void if_statement()
{
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition");

    int thenJump = emit_jump(OP_JUMP_IF_FALSE);
    // Condition is true, pop value
    emit_byte(OP_POP);
    statement();

    int elseJump = emit_jump(OP_JUMP);

    patch_jump(thenJump);
    // Condition is false, pop value
    emit_byte(OP_POP);

    // Optional else clause
    if (match(TOKEN_ELSE)) {
        statement();
    }
    // Jump to the next statement after else branch
    patch_jump(elseJump);
}

static void emit_jump_back(int offset)
{
    emit_byte(OP_JUMP_BACK);

    int jump = current_chunk()->count - offset + 2;
    if (jump > UINT16_MAX) {
        error("Loop body too large.");
    }

    emit_byte((jump >> 8) & 0xff);
    emit_byte(jump & 0xff);
}

static void while_statement()
{
    int start_loop = current_chunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition");

    int end_loop = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    emit_jump_back(start_loop);

    patch_jump(end_loop);
    emit_byte(OP_POP);
}

static void return_statement()
{
    if (g_compiler->type == CHUNK_MAIN) {
        error("Cannot return from main script");
    }

    if (match(TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after return expresson");
        emit_byte(OP_RETURN);
    }
}

static void declaration()
{
    if (match(TOKEN_LET)) {
        var_declaration(false);
    } else if (match(TOKEN_CONST)) {
        var_declaration(true);
    } else if (match(TOKEN_DEF)) {
        function_declaration();
    } else {
        statement();
    }

    // If we a hit a compilation error while parsing the previous statement,
    // we entered panic mode
    if (parser.panic_mode) {
        synchronize();
    }
}

static void block()
{
    while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void statement()
{
    if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_RETURN)) {
        return_statement();
    } else {
        expression_statement();
    }
}

static void rule_grouping(bool _assignable)
{
    (void)_assignable;
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static void rule_number(bool _assignable)
{
    (void)_assignable;
    double value = strtod(parser.previous.start, NULL);
    // Common values have dedicated op codes
    if (value == 0) {
        emit_byte(OP_ZERO);
    } else if (value == 1) {
        emit_byte(OP_ONE);
    } else {
        // Emit load instruction
        emit_constant(make_number(value));
    }
}

static void rule_string(bool _assignable)
{
    (void)_assignable;
    emit_constant(
        make_string_from_buffer(parser.previous.start + 1, parser.previous.length - 2));
}

static void rule_binary_op(bool _assignable)
{
    (void)_assignable;
    TokenType type = parser.previous.type;
    const ParseRule* rule = &rules[type];

    parse_precedence((Precedence)(rule->precedence + 1));
    switch (type) {
    case TOKEN_PLUS: emit_byte(OP_ADD); break;
    case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
    case TOKEN_STAR: emit_byte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
    case TOKEN_PERCENT: emit_byte(OP_MODULO); break;
    case TOKEN_BANG_EQUAL: emit_byte(OP_NOT_EQUAL); break;
    case TOKEN_EQUAL_EQUAL: emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER: emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_byte(OP_GREATER_EQUAL); break;
    case TOKEN_LESS: emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL: emit_byte(OP_LESS_EQUAL); break;
    default:
        return; // Unreachable
    }
}

static void rule_unary_op(bool _assignable)
{
    (void)_assignable;
    TokenType type = parser.previous.type;
    // Parse the operand
    parse_precedence(PREC_UNARY);

    // Emit instruction for the unary operator
    switch (type) {
    case TOKEN_BANG:
        emit_byte(OP_NOT);
        break;
    case TOKEN_PLUS:
        emit_byte(OP_POSITIVE);
        break;
    case TOKEN_MINUS:
        emit_byte(OP_NEGATIVE);
        break;
    default:
        return; // Unreachable
    }
}

static void rule_literal(bool _assignable)
{
    (void)_assignable;

    switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NULL: emit_byte(OP_NULL); break;
    case TOKEN_TRUE: emit_byte(OP_TRUE); break;
    default:
        return; // Unreachable
    }
}

static uint8_t argument_list()
{
    uint8_t arg_count = 0;
    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        do {
            expression();
            // Arity is stored on a single byte
            if (arg_count == UINT8_MAX) {
                error("Cannot handle more than 255 arguments");
            }
            ++arg_count;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after argument list");
    return arg_count;
}

static void rule_and(bool _assignable)
{
    (void)_assignable;
    int end_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(end_jump);
}

static void rule_or(bool _assignable)
{
    (void)_assignable;
    int end_jump = emit_jump(OP_JUMP_IF_TRUE);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static void rule_fn_call(bool _assignable)
{
    (void)_assignable;
    // Argument count of the function call is the operand to OP_CALL,
    // it is stored on a single byte
    uint8_t arg_count = argument_list();
    emit_byte(OP_CALL);
    emit_byte(arg_count);
}

static void rule_subscript(bool assignable)
{
    expression();
    consume(TOKEN_RIGHT_BRACKET, "Expected ']'");
    // Check if [] is followed by an assignment
    if (assignable && match(TOKEN_EQUAL)) {
        expression();
        emit_byte(OP_SUBSCRIPT_SET);
    } else {
        emit_byte(OP_SUBSCRIPT_GET);
    }
}

/**
 * Get local variable offset
 * @param name: name of the local
 * @return offset if local has been found, otherwise -1
 */
static int get_local_variable(Compiler* compiler, const Token* name)
{
    // Loop from the end: locals from inner scopes can shadow locals from outer
    // scopes
    for (int i = compiler->local_count - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                error("Cannot read local variable before its initialization");
            }
            return i;
        }
    }
    return -1;
}

/**
 * Emit GET/SET instructions for global/local variables
 */
static void rule_variable(bool assignable)
{
    // Get variable name
    const Token* token = &parser.previous;
    // Check local vs global
    int arg = get_local_variable(g_compiler, token);
    if (arg != -1) {
        // LOCAL VARIABLE
        // If the name is followed by =, this is an assignment (setter).
        // Otherwise, this is an identifier access (getter).
        if (assignable && match(TOKEN_EQUAL)) {
            if (g_compiler->locals[arg].read_only) {
                error("Cannot assign const local variable");
            } else {
                expression();
                emit_byte(OP_SET_LOCAL);
                emit_byte(arg);
            }
        } else {
            emit_byte(OP_GET_LOCAL);
            emit_byte(arg);
        }
    } else {
        // GLOBAL VARIABLE
        // Register the identifier name as a constant in the chunk
        int constant_index = chunk_register_constant(
            current_chunk(), make_string_from_buffer(token->start, token->length));
        if (constant_index > UINT16_MAX) {
            error("Cannot use more than UINT16_MAX constants");
        }
        if (assignable && match(TOKEN_EQUAL)) {
            expression();
            if (constant_index <= UINT8_MAX) {
                emit_byte(OP_SET_GLOBAL);
                emit_byte(constant_index);
            } else if (constant_index <= UINT16_MAX) {
                emit_byte(OP_SET_GLOBAL_16);
                emit_byte((constant_index >> 8) & 0xff);
                emit_byte(constant_index & 0xff);
            }
        } else {
            if (constant_index <= UINT8_MAX) {
                emit_byte(OP_GET_GLOBAL);
                emit_byte(constant_index);
            } else if (constant_index <= UINT16_MAX) {
                emit_byte(OP_GET_GLOBAL_16);
                emit_byte((constant_index >> 8) & 0xff);
                emit_byte(constant_index & 0xff);
            }
        }
    }
}

// Parser entry point
ObjectFunction* parser_compile(const char* source)
{
    parser.errored = false;
    parser.panic_mode = false;
    parser.source = source;

    scanner_init(source);
    Compiler compiler;
    compiler_init(&compiler, CHUNK_MAIN);

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjectFunction* function = end_compiler();
    return parser.errored ? NULL : function;
}
