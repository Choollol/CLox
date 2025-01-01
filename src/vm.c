#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../include/common.h"
#include "../include/compiler.h"
#include "../include/debug.h"
#include "../include/object.h"
#include "../include/value.h"
#include "../include/memory.h"
#include "../include/vm.h"

/// @returns Whether the top two values in the stack satisfy the given check macro/function.
#define CHECK_TOP_TWO(check) (check(peek(0)) && check(peek(1)))

VM vm;

/// @brief "Clear"s the VM's value stack by resetting the stackTop pointer.
static void resetStack() {
    vm.stackTop = vm.stack;
}

/// @brief Reports a runtime error.
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.strings);
}

void freeVM() {
    freeTable(&vm.strings);
    freeObjects();
}

/// @returns The value at the distance from the top of the stack, 0 for the value at the top.
static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

/// @brief Sets the stack's top value to the given value. Primarily used for unary operators.
static void setTopValue(Value value) {
    vm.stackTop[-1] = value;
}

/// @returns The falsiness of the given value.
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/// @brief Concatenates the first-from-stack-top string onto the end of the second-from-stack-top string and pushes it back onto the stack.
static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

/// @brief Returns the current instruction byte and increments the VM's instruction pointer.
#define READ_BYTE() (*vm.ip++)
/// @brief Reads a byte and returns the constant associated with that byte.
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
/// @brief Performs a binary operation on the top two values in the stack and pushes the result back onto the stack.
#define BINARY_OP(valueType, op)                          \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    } while (false)

/// @brief Helper method to print the VM's current value stack.
void printStack() {
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; ++slot) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
}

/// @brief Executes instructions in the VM.
static InterpretResult run() {
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        printStack();
        disassembleInstruction(vm.chunk, /* (int) */ (vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT:
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            case OP_NIL:
                push(NIL_VAL);
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_POP:
                pop();
                break;
            case OP_EQUAL:
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case OP_ADD:
                if (CHECK_TOP_TWO(IS_STRING)) {
                    concatenate();
                }
                else if (CHECK_TOP_TWO(IS_NUMBER)) {
                    BINARY_OP(NUMBER_VAL, +);
                }
                else {
                    runtimeError("Operands must be two numbers or strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case OP_NOT:
                setTopValue(BOOL_VAL(isFalsey(peek(0))));
                break;
            case OP_NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                setTopValue(NUMBER_VAL(-AS_NUMBER(vm.stackTop[-1])));
                break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_RETURN:
                // Exit interpreter
                return INTERPRET_OK;
        }
    }
}

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP

InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}

void push(Value value) {
    *vm.stackTop = value;
    ++vm.stackTop;
}

Value pop() {
    return *--vm.stackTop;
}