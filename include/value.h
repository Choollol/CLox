#ifndef CLOX_INCLUDE_VALUE_H
#define CLOX_INCLUDE_VALUE_H

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
} Value;

/// @returns Whether the given value holds a bool.
#define IS_BOOL(value) (value.type == VAL_BOOL)
/// @returns Whether the given value holds nil.
#define IS_NIL(value) (value.type == VAL_NIL)
/// @returns Whether the given value holds a number.
#define IS_NUMBER(value) (value.type == VAL_NUMBER)
/// @returns Whether the given value holds an object.
#define IS_OBJ(value) (value.type == VAL_OBJ)

/// @returns The bool held by the given value.
#define AS_BOOL(value) (value.as.boolean)
/// @returns The number held by the given value.
#define AS_NUMBER(value) (value.as.number)
/// @returns The object held by the given value.
#define AS_OBJ(value) (value.as.obj)

/// @returns A value constructed from the given bool.
#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
/// @returns A value constructed from nil.
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
/// @returns A value constructed from the given number.
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
/// @returns A value constructed from the given object.
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)(object)}})

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

/// @returns Whether the two given values are equal.
bool valuesEqual(Value a, Value b);

/// @brief Initializes an empty value array.
void initValueArray(ValueArray* array);
/// @brief Add a value to a value array.
void writeValueArray(ValueArray* array, Value value);
/// @brief Frees the given value array's memory.
void freeValueArray(ValueArray* array);

/// @brief Prints a value.
void printValue(Value value);

#endif