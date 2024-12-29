#ifndef CLOX_INCLUDE_VALUE_H
#define CLOX_INCLUDE_VALUE_H

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

#define IS_BOOL(value) (value.type == VAL_BOOL)
#define IS_NIL(value) (value.type == VAL_NIL)
#define IS_NUMBER(value) (value.type == VAL_NUMBER)

#define AS_BOOL(value) (value.as.boolean)
#define AS_NUMBER(value) (value.as.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

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