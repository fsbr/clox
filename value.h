#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;       // Nystrom only supports floats in ch14.

// why does nystrom like specific data structures?
// ValueArray is the constant pool
typedef struct {                                    
    int capacity;
    int count;
    Value* values;

} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

#endif
