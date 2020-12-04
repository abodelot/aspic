#ifndef ASPIC_SCANNER_H
#define ASPIC_SCANNER_H

typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN,  // (
    TOKEN_RIGHT_PAREN, // )
    TOKEN_LEFT_BRACE,  // {
    TOKEN_RIGHT_BRACE, // }
    TOKEN_COMMA,       // ,
    TOKEN_DOT,         // .
    TOKEN_MINUS,       // -
    TOKEN_PERCENT,     // %
    TOKEN_PLUS,        // +
    TOKEN_SEMICOLON,   // ;
    TOKEN_SLASH,       // /
    TOKEN_STAR,        // *
    // One or more character tokens
    TOKEN_AMPER,         // &
    TOKEN_AMPER_AMPER,   // &&
    TOKEN_PIPE,          // |
    TOKEN_PIPE_PIPE,     // ||
    TOKEN_BANG,          // !
    TOKEN_BANG_EQUAL,    // !=
    TOKEN_EQUAL,         // =
    TOKEN_EQUAL_EQUAL,   // ==
    TOKEN_GREATER,       // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS,          // <
    TOKEN_LESS_EQUAL,    // <=
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    // Keywords
    TOKEN_CLASS,
    TOKEN_CONST,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_LET,
    TOKEN_NULL,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_WHILE,

    TOKEN_ERROR, // Special token to handle unrecognized characters
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

/**
 * Ctor
 */
void scanner_init(const char* source);

/**
 * Read next token from the scanner
 */
Token next_token();

/**
 * Print token to stdout (debug)
 */
void print_token(const Token* token);

#endif
