#ifndef clox_vm_h                               // to prevent redundant/cyclical includes
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct{
    Chunk* chunk;
    uint8_t* ip;        // it is faster to dereference a pointer than look up an element in an array by index
    Value stack[STACK_MAX];
    Value* stackTop;            // IMPORTANT: the stack pointer points to one element ABOVE where the last item is.
} VM;

typedef enum {          // jayko has nothing equivalent to this
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();          // okay i'm kind of getting use to this pattern

InterpretResult interpret(const char* source);

void push(Value value);
Value pop();

#endif
