#ifndef CLOX_INCLUDE_OBJECT_H
#define CLOX_INCLUDE_OBJECT_H

#include "common.h"
#include "value.h"
#include "chunk.h"

/// @returns The ObjType of the object held by the given value.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

/// @returns Whether the given Value holds a closure object.
#define IS_CLOSURE(value) (isObjType(value, OBJ_CLOSURE))
/// @returns Whether the given Value holds a function object.
#define IS_FUNCTION(value) (isObjType(value, OBJ_FUNCTION))
/// @returns Whether the given Value holds a function object.
#define IS_NATIVE(value) (isObjType(value, OBJ_NATIVE))
/// @returns Whether the given Value holds a string object.
#define IS_STRING(value) (isObjType(value, OBJ_STRING))

/// @returns The closure object held by the given Value.
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
/// @returns The function object held by the given Value.
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
/// @returns The C function pointer from the native-function object held by the given Value.
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
/// @returns The string object held by the given Value.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
/// @returns The null-terminated c-string in the string object held by the given Value.
#define AS_CSTRING(value) (AS_STRING(value)->chars)

typedef enum ObjType {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj* next;
};

typedef struct {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value closed;
    Value* location;
    struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

/// @brief Creates a closure object that closes over the given function object.
ObjClosure* newClosure(ObjFunction* function);

/// @brief Creates an empty-initialized function.
ObjFunction* newFunction();
/// @brief A constructor-like function for creating native functions.
ObjNative* newNative(NativeFn function);

/// @returns An ObjString* that takes ownership of the given chars.
ObjString* takeString(char* chars, int length);
/// @brief Allocates a new string on the heap from the given string in the source code.
ObjString* copyString(const char* chars, int length);
/// @brief Constructor-like function for upvalue objects;
ObjUpvalue* newUpvalue(Value* slot);
/// @brief Prints a representation of an object value.
void printObject(Value value);

/// @returns Whether the given value holds an object of the given type.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif