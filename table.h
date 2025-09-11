#ifndef clox_table_h
#define clox_table_h


// Header file for HashMap implementation

#include "common.h"
#include "value.h"

typedef struct {
    // simple key value pair
    ObjString* key;
    Value value;
} Entry;

typedef struct {            // this pattern is so common in C
    int count;              // number of elements in the table
    int capacity;           // current size of the allocated array
    Entry* entries;         // pointer to whatever is actually in there
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);


#endif
