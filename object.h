#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
} ObjType;

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)

#define IS_FUNCTION(value)      isObjType(value, OBJ_FUNCTION)
#define IS_STRING(value)        isObjType(value, OBJ_STRING)


#define AS_FUNCTION(value)      ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value)        ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)       ((ObjString*)AS_OBJ(value))->chars

// we're doing this to prepare for a lot of heap allocation in the future
struct Obj {
    ObjType type;
    struct Obj* next;           // for gc purposes but i feel like theres some crazy stuff we might be able to do with this
};

// functions are first class... so they need to be actual Lox Objects
typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;              // for hashtable implementation
};


ObjFunction* newFunction();
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);


// since this expression uses "value" twice, and value is mutable, we don't just put it into the 
// IS_STRING macro. we have this funtion to maintain immutability
// I will not remember this and fall for it most likely. 
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type  == type;
}

#endif
