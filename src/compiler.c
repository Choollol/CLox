#include <stdio.h>
#include <stdlib.h>

#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/scanner.h"
#include "../include/object.h"

#ifdef DEBUG_PRINT_CODE
#include "../include/debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

/// @returns The chunk that is currently being compiled.
static Chunk* currentChunk() {
    return compilingChunk;
}

/// @brief Reports an error at the given token.
static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) {
        return;
    }

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, " at %.*s", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}
/// @brief Reports an error at the parser's previous token.
static void error(const char* message) {
    errorAt(&parser.previous, message);
}
/// @brief Reports an error at the parser's current token.
static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

/// @brief Advances to the next token.
static void advance() {
    parser.previous = parser.current;

    while (true) {
        parser.current = scanToken();

        if (parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser.current.start);
    }
}
/// @brief If the current token matches the given type, advance. Otherwise, report an error.
static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

/// @brief Appends a single byte to the current chunk.
static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}
/// @brief Appends two bytes to the current chunk.
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}
/// @brief Appends a return instruction to the current chunk.
static void emitReturn() {
    emitByte(OP_RETURN);
}

/// @brief Adds a constant to the VM's constant table and checks for overflow.
static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);

    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}
/// @brief Appends a constant to the current chunk.
static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

/// @brief Ends compilation.
static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

/// @brief Parses the expressions at the parser's current token that are equal to or higher than the given precedence.
static void parsePrecedence(Precedence precedence);
/// @brief Parses an expression.
static void expression();
/// @returns A pointer to the ParseRule corresponding to the given TokenType.
static ParseRule* getRule(TokenType type);

/// @brief Parses a binary expression.
static void binary() {
    TokenType operatorType = parser.previous.type;

    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
    }
}

/// @brief Parses a non-number literal expression.
static void literal() {
    switch (parser.previous.type) {
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
        default:
            // Unreachable
            return;
    }
}

/// @brief Parses a grouped expression. Assumes the opening parenthesis has been consumed already.
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/// @brief Parses a number literal.
static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/// @brief Parses a string literal.
static void string() {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

/// @brief Parses a unary expression.
static void unary() {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        default:
            // Unreachable
            return;
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;

    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression");

    endCompiler();

    return !parser.hadError;
}