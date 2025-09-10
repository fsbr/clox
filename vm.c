#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;   // since pointers are just addresses this is possible
}

static void runtimeError(const char* format, ...) {
    va_list args;                                       // va here is a special type
    va_start(args, format);                             // that helps us implement
    vfprintf(stderr, format, args);                     // variadic functions
    va_end(args);                                       // enjoy it!
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code -1;     // the code being executed is one behind the actually -> code
    int line = vm.chunk->lines[instruction];            // eventually we will implement a call stack and have it trace thru here
    fprintf(stderr, "[line %d] in script \n", line);
    resetStack();

}

void initVM() {
    resetStack();
}

// what an amazing implementation
void freeVM() {

}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    // since the stack top is one element above the first valid item, you decrement once
    // to get to the valid item
    return vm.stackTop[-1 - distance];      // 
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// "This is the single most important function in all of clox by far"
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
    } while (0)

//#define BINARY_OP(op) \
//    do { \
//        double b = pop(); \
//        double a = pop(); \
//        push( a op b ); \
//    } while (0)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
    }
    printf("\n");
    disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));        // it isnt clear to me why the program code isn't just in one huge chunk.... there's going to be multiple chunks maybe.
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a,b)));
                break;
            }

            case OP_GREATER:   BINARY_OP(BOOL_VAL, >);   break;
            case OP_LESS:   BINARY_OP(BOOL_VAL, <);   break;


            case OP_ADD:        BINARY_OP(NUMBER_VAL, +); break;    // use C's macro system to impelment a string literal like that.
            case OP_SUBTRACT:   BINARY_OP(NUMBER_VAL, -); break;    // that feels pretty powerful/dangerous
            case OP_MULTIPLY:   BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:     BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;

            case OP_NEGATE: 
                if (!IS_NUMBER( peek(0) )) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP

}

InterpretResult interpret(const char* source) {
    // beginning of ch 17:
    Chunk chunk;
    initChunk(&chunk);

    if ( !compile(source, &chunk) ) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
    //compile(source);
    //return INTERPRET_OK;
    //vm.chunk = chunk;
    //vm.ip = vm.chunk->code;   // instruction pointer, points to the instruction about to be executed.
    //return run();             // what will actually run the bytecode instructions
                              // bob knows how gcc optimizes stuff, and builds his code around that.
}
