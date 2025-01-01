#include <ctype.h>
#include <string.h>

#include "../include/common.h"
#include "../include/scanner.h"

#ifdef DEBUG_PRINT
#include <stdio.h>
#endif

typedef struct {
    const char* start;    // Beginning of current lexeme
    const char* current;  // Current character
    int line;
} Scanner;

Scanner scanner;

/// @brief Initialize the global scanner.
void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

// Helper functions
/// @brief Check whether the end of the source code has been reached.
static bool isAtEnd() {
    return *scanner.current == '\0';
}
/// @brief Returns current character and advances to the next character.
static char advance() {
    return *scanner.current++;
}
/// @returns The current character the scanner is pointing at.
static char peek() {
    return *scanner.current;
}
/// @returns The character after the current character.
static char peekNext() {
    if (isAtEnd()) {
        return '\0';
    }
    return *(scanner.current + 1);
}
/// @returns True if the current character matches the given character.
static bool match(char expected) {
    if (isAtEnd() || peek() != expected) {
        return false;
    }
    advance();
    return true;
}
/// @brief Returns whether the given character can be the first character in an identifier.
static bool isCharValidIdFirst(char c) {
    return isalpha(c) || c == '_';
}

/// @brief Creates a token from the given type.
Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = scanner.current - scanner.start;
    token.line = scanner.line;
    return token;
}
/// @brief Creates a token with type TOKEN_ERROR and the given message.
Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner.line;
    return token;
}

/// @brief Skips whitespace and comments.
void skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case '\n':
                ++scanner.line;
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                }
                else if (peekNext() == '*') {
                    advance();
                    while (!isAtEnd()) {
                        if (match('*') && match('/')) {
                            break;
                        }
                        advance();
                    }
                }
                else {
                    return;
                }
            default:
                return;
        }
    }
}

/// @brief Scans a string literal.
static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            ++scanner.line;
        }
        advance();
    }
    if (isAtEnd()) {
        return errorToken("Unterminated string literal.");
    }
    // Closing quote
    advance();
    return makeToken(TOKEN_STRING);
}
/// @brief Scans a number literal.
static Token number() {
    while (isdigit(peek()) && !isAtEnd()) {
        advance();
    }
    if (peek() == '.') {
        advance();
        while (isdigit(peek()) && !isAtEnd()) {
            advance();
        }
    }
    return makeToken(TOKEN_NUMBER);
}

/// @brief Checks whether the rest of an identifier matches a keyword's.
/// @param start Offset from beginning of the current token.
/// @param length Length of remaining string to check.
/// @param remaining Remaining string to check.
/// @param type Type to return if remaining string is matched.
/// @return The given keyword TokenType if the remainder of the identifier matches. Otherwise, TOKEN_IDENTIFIER.
TokenType checkKeyword(int start, const char* remaining, TokenType type) {
    size_t length = strlen(remaining);
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, remaining, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}
/// @brief
TokenType identifierType() {
    int startOffset = 1;
    switch (*scanner.start) {
        // Straight paths
        case 'a':
            return checkKeyword(startOffset, "nd", TOKEN_AND);
        case 'c':
            return checkKeyword(startOffset, "lass", TOKEN_CLASS);
        case 'e':
            return checkKeyword(startOffset, "lse", TOKEN_ELSE);
        case 'i':
            return checkKeyword(startOffset, "f", TOKEN_IF);
        case 'n':
            return checkKeyword(startOffset, "il", TOKEN_NIL);
        case 'o':
            return checkKeyword(startOffset, "r", TOKEN_OR);
        case 'p':
            return checkKeyword(startOffset, "rint", TOKEN_PRINT);
        case 'r':
            return checkKeyword(startOffset, "eturn", TOKEN_RETURN);
        case 's':
            return checkKeyword(startOffset, "uper", TOKEN_SUPER);
        case 'v':
            return checkKeyword(startOffset, "ar", TOKEN_VAR);
        case 'w':
            return checkKeyword(startOffset, "hile", TOKEN_WHILE);

        // Branching paths
        case 'f':
            if (scanner.current - scanner.start > 1) {
                ++startOffset;
                switch (scanner.start[1]) {
                    case 'a':
                        return checkKeyword(startOffset, "lse", TOKEN_FALSE);
                    case 'o':
                        return checkKeyword(startOffset, "r", TOKEN_FOR);
                    case 'u':
                        return checkKeyword(startOffset, "n", TOKEN_FUN);
                }
            }
        case 't':
            if (scanner.current - scanner.start > 1) {
                ++startOffset;
                switch (scanner.start[1]) {
                    case 'h':
                        return checkKeyword(startOffset, "is", TOKEN_THIS);
                    case 'r':
                        return checkKeyword(startOffset, "ue", TOKEN_TRUE);
                }
            }
    }
    return TOKEN_IDENTIFIER;
}
/// @brief Scans an identifier.
static Token identifier() {
    while (isCharValidIdFirst(peek()) || isdigit(peek())) {
        advance();
    }
    return makeToken(identifierType());
}

/// Scans the next token and returns it.
Token scanToken() {
    skipWhitespace();

    scanner.start = scanner.current;

    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();

    if (isdigit(c)) {
        return number();
    }
    if (isCharValidIdFirst(c)) {
        return identifier();
    }

    switch (c) {
        // Single-character tokens
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);

        // Possibly double-character tokens
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

        // String literal
        case '"':
            return string();
    }

    return errorToken("Unexpected character.");
}