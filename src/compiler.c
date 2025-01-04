#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/object.h"
#include "../include/scanner.h"

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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
} Local;

typedef enum FunctionType {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
} FunctionType;

typedef struct Compiler {
    struct Compiler* enclosing;
    
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

Parser parser;
Compiler* current = NULL;

/// @returns The chunk that is currently being compiled.
static Chunk* currentChunk() {
    return &current->function->chunk;
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
/// @returns Whether the parser's current token's type is equal to the given type.
static bool check(TokenType type) {
    return parser.current.type == type;
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
    if (check(type)) {
        advance();
        return;
    }
    errorAtCurrent(message);
}
/// @brief If the parser's current token's type matches the given type, advances and return true. Otherwise, do nothing and return false.
static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
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
/// @brief Emits an instruction to jump back to the given offset.
static int emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emitBytes((offset >> 8) & 0xff, offset & 0xff);
}
/// @brief Emits a placeholder 16-bit jump-offset operand.
/// @returns Chunk offset of the emitted instruction.
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitBytes(0xff, 0xff);
    return currentChunk()->count - 2;
}
/// @brief Appends a return instruction to the current chunk.
static void emitReturn() {
    emitByte(OP_NIL);
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

/// @brief Fixes the actual jump offset in the instructions.
static void patchJump(int offset) {
    // -2 to account for jump-offset instructions
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

/// @brief Initializes the compiler.
static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;

    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

/// @brief Ends compilation.
static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

/// @brief Begins a new scope by incrementing the current compiler's scope depth.
static void beginScope() {
    ++current->scopeDepth;
}
/// @brief Ends a scope by decrementing the current compiler's scope depth.
static void endScope() {
    --current->scopeDepth;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        --current->localCount;
        emitByte(OP_POP);
    }
}

/// @returns A pointer to the ParseRule corresponding to the given TokenType.
static ParseRule* getRule(TokenType type);
/// @brief Parses the expressions at the parser's current token that are equal to or higher than the given precedence.
static void parsePrecedence(Precedence precedence);
/// @brief Parses an expression.
static void expression();
/// @brief Parses a declaration statement.
static void declaration();
/// @brief Parses a statement.
static void statement();

/// @brief Puts an identifier into the VM's constant table.
/// @returns The index of the constant in the constant table.
static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

/// @returns Whether the two given identifiers are equal.
static bool identifiersEqual(Token* a, Token* b) {
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

/// @returns Index of the given identifier in the compiler's local variables. If not found, returns -1.
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; --i) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(&local->name, name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

/// @brief Adds a local variable to the compiler's stack.
static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}
/// @brief Declares a local variable.
static void declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }

    Token* name = &parser.previous;

    for (int i = current->localCount - 1; i >= 0; --i) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}
/// @brief Parses a variable identifier and creates a constant for the name.
/// @returns The index of the variable's identifier in the constant table.
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser.previous);
}
/// @brief Marks the most recent local variable declaration as being initialized.
static void markInitialized() {
    if (current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}
/// @brief Parses a variable definition.
static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}
/// @brief Parses an argument list to a call.
/// @returns The number of arguments.
static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            ++argCount;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after argument list.");
    return argCount;
}

/// @brief Parses a logical AND expression.
static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}
/// @brief Parses a logical OR expression.
static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

/// @brief Parses a binary expression.
static void binary(bool canAssign) {
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

/// @brief Parses a call expression.
static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

/// @brief Parses a non-number literal expression.
static void literal(bool canAssign) {
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
static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/// @brief Parses a number literal.
static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

/// @brief Parses a string literal.
static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

/// @brief Parses the use of a named variable.
static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    }
    else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    }
    else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

/// @brief Parses the use of a variable.
static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

/// @brief Parses a unary expression.
static void unary(bool canAssign) {
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
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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

/// @brief Parses a block statement. Assumes the opening brace has already been consumed.
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/// @brief Parses a function definition.
static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            ++current->function->arity;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction* function = endCompiler();
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}
/// @brief Parses a function declaration. Assums the fun token has already been consumed.
static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}
/// @brief Parses a variable declaration. Assumes the var token has already been consumed.
static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    }
    else {
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

/// @brief Parses an expression statement.
static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression statement.");
    emitByte(OP_POP);
}
/// @brief Parses an if statement.
static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    // Then-branch
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);

    emitByte(OP_POP);

    if (match(TOKEN_ELSE)) {
        statement();
    }

    patchJump(elseJump);
}
/// @brief Parses a print statement. Assumes the print token has already been consumed.
static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print statement.");
    emitByte(OP_PRINT);
}
/// @brief Parses a return statement.
static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
        return;
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    }
    else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}
/// @brief Parses a while loop.
static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after while condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}
/// @brief Parses a for loop.
static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    // Initializer
    if (match(TOKEN_SEMICOLON)) {
        // No initializer
    }
    else if (match(TOKEN_VAR)) {
        varDeclaration();
    }
    else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;

    // Condition
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after for condition.");

        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    // Increment
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;

        expression();
        emitByte(OP_POP);

        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    endScope();
}

/// @brief Recover the parser after entering panic mode.
static void synchronize() {
    parser.panicMode = false;

    while (!check(TOKEN_EOF)) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
        }
        advance();
    }
}

static void declaration() {
    if (match(TOKEN_FUN)) {
        funDeclaration();
    }
    else if (match(TOKEN_VAR)) {
        varDeclaration();
    }
    else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}
static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    }
    else if (match(TOKEN_IF)) {
        ifStatement();
    }
    else if (match(TOKEN_RETURN)) {
        returnStatement();
    }
    else if (match(TOKEN_WHILE)) {
        whileStatement();
    }
    else if (match(TOKEN_FOR)) {
        forStatement();
    }
    else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    }
    else {
        expressionStatement();
    }
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;

    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}
static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

ObjFunction* compile(const char* source) {
    initScanner(source);

    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_EOF, "Expect end of expression");

    ObjFunction* function = endCompiler();

    return parser.hadError ? NULL : function;
}