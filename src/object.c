#include <string.h>
#include <stdio.h>

#include "../include/object.h"
#include "../include/memory.h"
#include "../include/vm.h"

/// @brief Allocates an object on the heap, a sort of constructor.
/// @returns An Obj* to the allocated object.
#define ALLOCATE_OBJ(type, objectType) ((type*)allocateObject(sizeof(type), objectType))

/// @brief Allocates an object on the heap.
static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

/// @brief Allocates a string object on the heap.
static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    return string;
}

ObjString* takeString(char* chars, int length) {
    return allocateString(chars, length);
}

ObjString* copyString(const char* chars, int length) {
    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}