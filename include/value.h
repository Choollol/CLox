#ifndef CLOX_INCLUDE_VALUE_H
#define CLOX_INCLUDE_VALUE_H

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)

#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3

typedef uint64_t Value;

/// @returns Whether the given value holds a bool.
#define IS_BOOL(value) ((value | 1) == TRUE_VAL)
/// @returns Whether the given value holds nil.
#define IS_NIL(value) (value == NIL_VAL)
/// @returns Whether the given value holds a number.
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
/// @returns Whether the given value holds an object.
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

/// @returns The bool held by the given value.
#define AS_BOOL(value) (value == TRUE_VAL)
/// @returns The number held by the given value.
#define AS_NUMBER(value) valueToNum(value)
/// @returns The object held by the given value.
#define AS_OBJ(value) ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

/// @returns A value constructed from the given bool.
#define BOOL_VAL(value) ((value) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
/// @returns A value constructed from nil.
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
/// @returns A value constructed from the given number.
#define NUMBER_VAL(num) numToValue(num)
/// @returns A value constructed from the given object.
#define OBJ_VAL(object) (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(object))

/// @brief reinterpret_cast-like for doubles to Values.
static inline Value numToValue(double num) {
    union {
        Value value;
        double num;
    } data;
    data.num = num;
    return data.value;
}
/// @brief reinterpret_cast-like for Values to doubles.
static inline double valueToNum(Value value) {
    union {
        Value value;
        double num;
    } data;
    data.value = value;
    return data.num;
}

#else

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

#endif

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