#include "parser.h"
#include "chunk.h"
#include "debug.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    Token current;
    Token previous;
    bool errored;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // &&
    PREC_AND,        // ||
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * / %
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

static void parse_grouping(bool);
static void parse_unary_op(bool);
static void parse_binary_op(bool);
static void parse_number(bool);
static void parse_string(bool);
static void parse_literal(bool);
static void parse_fn_call(bool);
static void parse_variable(bool);

ParseRule rules[] = {
    // IMPORTANT: keep synced with TokenType enum
    // Single-character tokens
    [TOKEN_LEFT_PAREN] = { parse_grouping, parse_fn_call, PREC_CALL },
    [TOKEN_RIGHT_PAREN] = { NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_RIGHT_BRACE] = { NULL, NULL, PREC_NONE },
    [TOKEN_COMMA] = { NULL, NULL, PREC_NONE },
    [TOKEN_DOT] = { NULL, NULL, PREC_NONE },
    [TOKEN_MINUS] = { parse_unary_op, parse_binary_op, PREC_TERM },
    [TOKEN_PERCENT] = { NULL, parse_binary_op, PREC_FACTOR },
    [TOKEN_PLUS] = { parse_unary_op, parse_binary_op, PREC_TERM },
    [TOKEN_SEMICOLON] = { NULL, NULL, PREC_NONE },
    [TOKEN_SLASH] = { NULL, parse_binary_op, PREC_FACTOR },
    // One or more character tokens
    [TOKEN_AMPER] = { NULL, NULL, PREC_NONE },
    [TOKEN_AMPER_AMPER] = { NULL, NULL, PREC_NONE },
    [TOKEN_PIPE] = { NULL, NULL, PREC_NONE },
    [TOKEN_PIPE_PIPE] = { NULL, NULL, PREC_NONE },
    [TOKEN_STAR] = { NULL, parse_binary_op, PREC_FACTOR },
    [TOKEN_BANG] = { parse_unary_op, NULL, PREC_NONE },
    [TOKEN_BANG_EQUAL] = { NULL, parse_binary_op, PREC_EQUALITY },
    [TOKEN_EQUAL] = { NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL_EQUAL] = { NULL, parse_binary_op, PREC_EQUALITY },
    [TOKEN_GREATER] = { NULL, parse_binary_op, PREC_COMPARISON },
    [TOKEN_GREATER_EQUAL] = { NULL, parse_binary_op, PREC_COMPARISON },
    [TOKEN_LESS] = { NULL, parse_binary_op, PREC_COMPARISON },
    [TOKEN_LESS_EQUAL] = { NULL, parse_binary_op, PREC_COMPARISON },
    // Literals
    [TOKEN_IDENTIFIER] = { parse_variable, NULL, PREC_NONE },
    [TOKEN_STRING] = { parse_string, NULL, PREC_NONE },
    [TOKEN_NUMBER] = { parse_number, NULL, PREC_NONE },
    // Keywords
    [TOKEN_CLASS] = { NULL, NULL, PREC_NONE },
    [TOKEN_CONST] = { NULL, NULL, PREC_NONE },
    [TOKEN_ELSE] = { NULL, NULL, PREC_NONE },
    [TOKEN_FALSE] = { parse_literal, NULL, PREC_NONE },
    [TOKEN_FOR] = { NULL, NULL, PREC_NONE },
    [TOKEN_FUN] = { NULL, NULL, PREC_NONE },
    [TOKEN_IF] = { NULL, NULL, PREC_NONE },
    [TOKEN_LET] = { NULL, NULL, PREC_NONE },
    [TOKEN_NULL] = { parse_literal, NULL, PREC_NONE },
    [TOKEN_RETURN] = { NULL, NULL, PREC_NONE },
    [TOKEN_SUPER] = { NULL, NULL, PREC_NONE },
    [TOKEN_THIS] = { NULL, NULL, PREC_NONE },
    [TOKEN_TRUE] = { parse_literal, NULL, PREC_NONE },
    [TOKEN_WHILE] = { NULL, NULL, PREC_NONE },
    [TOKEN_ERROR] = { NULL, NULL, PREC_NONE },
    [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
};

Parser parser;
Chunk* g_current_chunk;

static Chunk* current_chunk()
{
    return g_current_chunk;
}

static void error_at(Token* token, const char* message)
{
    // If the panic_mode flag is already set, suppress any other errors
    // that get detected. Otherwise, turn on panic_mode flag.
    if (parser.panic_mode) {
        return;
    }
    parser.panic_mode = true;
    fprintf(stderr, "SyntaxError at line %d:\n    ", token->line);
    chunk_print_line(current_chunk(), token->line);

    fprintf(stderr, "%s", message);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at EOF\n");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
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
    emit_byte(OP_RETURN);
}

static int emit_constant(Value constant)
{
    // Register the constant value in the chunk
    Chunk* chunk = current_chunk();
    int constant_index = chunk_add_constant(chunk, constant);

    // Write load instruction + index
    if (!chunk_write_constant(current_chunk(), constant_index, parser.previous.line)) {
        error("Too many constants in one chunk");
    }
    return constant_index;
}

static void end_compiler()
{
    emit_return();
#if ASPIC_DEBUG
    if (!parser.errored) {
        chunk_dump(current_chunk(), "chunk");
    }
#endif
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
        case TOKEN_FUN:
        case TOKEN_FOR:
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

static uint8_t emit_variable(const char* error)
{
    consume(TOKEN_IDENTIFIER, error);

    // Register the identifer as a constant value in the chunk
    Value identifier = make_string_from_buffer(parser.previous.start, parser.previous.length);
    Chunk* chunk = current_chunk();
    return chunk_add_constant(chunk, identifier);
}

static void var_statement(bool const_lock)
{
    uint8_t global = emit_variable("Expected variable name");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NULL);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");

    // Declare variable
    emit_byte(const_lock ? OP_DECL_GLOBAL_CONST : OP_DECL_GLOBAL);
    emit_byte(global);
}

static void statement()
{
    expression_statement();
}

static void declaration()
{
    if (match(TOKEN_LET)) {
        var_statement(false);
    } else if (match(TOKEN_CONST)) {
        var_statement(true);
    } else {
        statement();
    }

    // If we a hit a compilation error while parsing the previous statement,
    // we entered panic mode
    if (parser.panic_mode) {
        synchronize();
    }
}

static void parse_grouping(bool _assignable)
{
    (void)_assignable;
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static void parse_number(bool _assignable)
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

static void parse_string(bool _assignable)
{
    (void)_assignable;
    emit_constant(
        make_string_from_buffer(parser.previous.start + 1, parser.previous.length - 2));
}

static void parse_binary_op(bool _assignable)
{
    (void)_assignable;
    TokenType type = parser.previous.type;
    ParseRule* rule = &rules[type];

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

static void parse_unary_op(bool _assignable)
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

static void parse_literal(bool _assignable)
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
            ++arg_count;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after argument list");
    return arg_count;
}

static void parse_fn_call(bool _assignable)
{
    (void)_assignable;
    // Argument count of the function call is the operand to OP_CALL,
    // it is stored on a single byte
    uint8_t arg_count = argument_list();
    emit_byte(OP_CALL);
    emit_byte(arg_count);
}

static void named_variable(Token* token, bool assignable)
{
    // Register the identifier name as a constant in the chunk
    uint8_t constant_index = chunk_add_constant(
        current_chunk(), make_string_from_buffer(token->start, token->length));

    // If the identifier is followed by =, this is an assignment (setter).
    // Otherwise, this is an identifier access (getter).
    if (assignable && match(TOKEN_EQUAL)) {
        expression();
        emit_byte(OP_SET_GLOBAL);
    } else {
        emit_byte(OP_GET_GLOBAL);
    }
    emit_byte(constant_index);
}

static void parse_variable(bool assignable)
{
    named_variable(&parser.previous, assignable);
}

// Parser entry point
bool parser_compile(Chunk* chunk)
{
    parser.errored = false;
    parser.panic_mode = false;

    scanner_init(chunk->source);
    g_current_chunk = chunk;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    end_compiler();
    return !parser.errored;
}
