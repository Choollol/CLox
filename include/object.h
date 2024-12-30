#ifndef CLOX_INCLUDE_OBJECT_H
#define CLOX_INCLUDE_OBJECT_H

#include "common.h"
#include "value.h"

/// @returns The ObjType of the object held by the given value.
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

/// @returns Whether the given value holds a string object.
#define IS_STRING(value) (isObjType(value, OBJ_STRING))

/// @returns The string object held by the given value.
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
/// @returns The null-terminated c-string in the string object held by the given value.
#define AS_CSTRING(value) (AS_STRING(value)->chars)

typedef enum ObjType {
    OBJ_STRING,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
};

/// @returns An ObjString* that takes ownership of the given chars.
ObjString* takeString(char* chars, int length);
/// @brief Allocates a new string on the heap from the given string in the source code.
ObjString* copyString(const char* chars, int length);
/// @brief Prints a representation of an object value.
void printObject(Value value);

/// @returns Whether the given value holds an object of the given type.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif