#ifndef clox_vm_h                               // to prevent redundant/cyclical includes
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct{
    Chunk* chunk;               // a chunk is where all the bytecode is stored.
    uint8_t* ip;                // it is faster to dereference a pointer than look up an element in an array by index
    Value stack[STACK_MAX];     // The stack sized is fixed right now, but in theory we could make it a dynamic array
    Value* stackTop;            // IMPORTANT: the stack pointer points to one element ABOVE where the last item is.
    Table strings;
    Obj* objects;               // pointer to the head of the list of objects
} VM;

typedef enum {                  // jayko has nothing equivalent to this
    INTERPRET_OK,               // we detected compilation to go will
    INTERPRET_COMPILE_ERROR,    // we detect a compilation error
    INTERPRET_RUNTIME_ERROR     // we detect a runtime error
} InterpretResult;


extern VM vm;                   // expose the vm globally (extern keyword)

// Function prototypes
void initVM();          // initialize and free memory dedicated to the VM
void freeVM();          // okay i'm kind of getting use to this pattern

InterpretResult interpret(const char* source);          // interpeter

// Since our vm is a stack, we have stack push and pop operations
void push(Value value);
Value pop();

#endif
