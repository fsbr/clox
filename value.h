#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// Value types that have built in support from the VM are separate from classes.
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
    } as;                   // you read it as "type as bool" or "type as "double"
} Value;

// Type checking
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)


// these macros access into the value typedef struct
// Take the value in the struct back to "C" from "clox".  I guess..
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

// side note i love that you can just type cast and {a, b } to declare into a struct
// place the correct value into the Value typedef stcut

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number= value}})


// why does nystrom like specific data structures?
// ValueArray is the constant pool
typedef struct {                                    
    int capacity;
    int count;
    Value* values;

} ValueArray;

bool valuesEqual(Value a, Value b);

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
