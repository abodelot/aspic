#include "scanner.h"
#include "shared.h"

#include <stdio.h>
#include <string.h>

#define COMMENT_CHAR '#'

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

// Consume next character if it matches the given expected character
static bool match(char expected)
{
    if (*scanner.current == '\0') {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }

    scanner.current++;
    return true;
}

// Consume every whitespace character
static void skip_whitespaces()
{
    for (;;) {
        switch (*scanner.current) {
        case ' ':
        case '\r':
        case '\t':
            ++scanner.current;
            break;
        case '\n':
            ++scanner.line;
            ++scanner.current;
            break;
        case COMMENT_CHAR:
            // Ignore all characters until EOL or EOF
            while (*scanner.current != '\n' && *scanner.current != '\0') {
                ++scanner.current;
            }
            break;
        default:
            return;
        }
    }
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/**
 * Check if the current identifier matches a given string
 * @param start: ignore the first 'start' characters of the current identifier
 * @param rest: check the current identifier against 'rest'
 * @param type: TokenType to be returned if rest matches the current identifier
 * @return type if match is successful, othersewise TOKEN_IDENTIFIER
 */
static TokenType check_keyword(int start, const char* rest, TokenType type)
{
    int length = strlen(rest);
    if (scanner.current - scanner.start == start + length
        && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

/**
 * If the current identifier is a keyword, return associated TokenType
 * Otherwise, return TOKEN_IDENTIFIER
 */
static TokenType get_identifier_type()
{
    // Check the initial letters which can start a keyword
    switch (*scanner.start) {
    case 'c':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'l': return check_keyword(2, "ass", TOKEN_CLASS);
            case 'o': return check_keyword(2, "nst", TOKEN_CONST);
            }
        }
        break;
    case 'e': return check_keyword(1, "lse", TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'a': return check_keyword(2, "lse", TOKEN_FALSE);
            case 'u': return check_keyword(2, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i': return check_keyword(1, "f", TOKEN_IF);
    case 'l': return check_keyword(1, "et", TOKEN_LET);
    case 'n': return check_keyword(1, "ull", TOKEN_NULL);
    case 'r': return check_keyword(1, "eturn", TOKEN_RETURN);
    case 's': return check_keyword(1, "uper", TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1) {
            switch (scanner.start[1]) {
            case 'h': return check_keyword(2, "is", TOKEN_THIS);
            case 'r': return check_keyword(2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'w': return check_keyword(1, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token make_token(TokenType type)
{
    return (Token) {
        .type = type,
        .start = scanner.start,
        .length = (int)(scanner.current - scanner.start),
        .line = scanner.line,
    };
}

static Token make_error_token(const char* message)
{
    return (Token) {
        .type = TOKEN_ERROR,
        .start = message,
        .length = strlen(message),
        .line = scanner.line,
    };
}

void scanner_init(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

Token scan_string()
{
    char c = *scanner.current;
    // Consume characters until closing quote or EOF is reached
    while (c != '"' && c != '\0') {
        // Allow multi-line string, but don't forget to increment lineno!
        if (c == '\n') {
            ++scanner.line;
        }
        c = *++scanner.current;
    }

    // EOF is reached but closing quote is missing
    if (*scanner.current == '\0') {
        return make_error_token("Unterminated string");
    }

    // Consume the closing quote
    ++scanner.current;
    return make_token(TOKEN_STRING);
}

Token scan_number()
{
    while (is_digit(*scanner.current)) {
        ++scanner.current;
    }

    // '.' is valid only when followed by another digit
    // 123.x => KO
    // 123.4 => OK
    if (*scanner.current == '.' && is_digit(scanner.current[1])) {
        ++scanner.current;
        while (is_digit(*scanner.current)) {
            ++scanner.current;
        }
    }

    return make_token(TOKEN_NUMBER);
}

Token scan_identifier()
{
    while (is_alpha(*scanner.current) || is_digit(*scanner.current)) {
        ++scanner.current;
    }
    return make_token(get_identifier_type());
}

Token next_token()
{
    skip_whitespaces();
    scanner.start = scanner.current;

    if (*scanner.current == '\0') {
        return make_token(TOKEN_EOF);
    }

    char c = *scanner.current++;
    if (is_alpha(c)) {
        return scan_identifier();
    }
    if (is_digit(c)) {
        return scan_number();
    }
    switch (c) {
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}': return make_token(TOKEN_RIGHT_BRACE);
    case '[': return make_token(TOKEN_LEFT_BRACKET);
    case ']': return make_token(TOKEN_RIGHT_BRACKET);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ',': return make_token(TOKEN_COMMA);
    case '.': return make_token(TOKEN_DOT);
    case '-': return make_token(TOKEN_MINUS);
    case '+': return make_token(TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
    case '%': return make_token(TOKEN_PERCENT);
    case '&': return make_token(match('&') ? TOKEN_AMPER_AMPER : TOKEN_AMPER);
    case '|': return make_token(match('|') ? TOKEN_PIPE_PIPE : TOKEN_PIPE);
    case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return scan_string();
    }

    return make_error_token("Unexpected character");
}

void print_token(const Token* token)
{
    // Pass the precision as argument with *: prints the first token.length
    // characters, starting at token.start
    if (token->type == TOKEN_EOF) {
        printf("<EOF>");
    } else {
        printf("%2d '%.*s'", token->type, token->length, token->start);
    }
}
