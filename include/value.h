#ifndef CLOX_INCLUDE_VALUE_H
#define CLOX_INCLUDE_VALUE_H

typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

/// @brief Initializes an empty value array.
void initValueArray(ValueArray* array);
/// @brief Add a value to a value array.
void writeValueArray(ValueArray* array, Value value);
/// @brief Frees the given value array's memory.
void freeValueArray(ValueArray* array);

/// @brief Prints a value.
void printValue(Value value);

#endif