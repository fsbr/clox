#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"


typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

// in jayko we had it as attributes on the tokens, which sucked.
typedef enum {                  
    PREC_NONE,                      // LOWEST PRECEDENCE
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == != 
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY,    //             // HIGHEST PRECEDENCE
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;             // the nud()
    ParseFn infix;              // the led()
    Precedence precedence;      // the right binding power
} ParseRule;


Parser parser;                                  // remember for a "real" language this is bad. 

Chunk* compilingChunk;


static Chunk* currentChunk() {                  // this is like where im stuck on jayko.
    return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing yet
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start); // this formatting string was in last chapter too!
    }

    fprintf(stderr, ": %s\n", message);    
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}


static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}


static void advance() {
    parser.previous = parser.current;           // a stash of our previous token now that we've advanced it!, so i need to understand all the diff match /expect peek etc logic in my program.

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char* message) { // in jayko i think we called this one "expect"
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line); // TYPING parser.previous.line here because the author put it here is one thing but acutally implementing it is like damn.
}

static void emitBytes(uint8_t byte1, uint8_t byte2) { 
    emitByte(byte1);            // it is often the case that we have an opcode byte1, and an operand
    emitByte(byte2);            // byte2, so this function is to help with that.
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX){
        error("Too many constants in one chunk.");           // it returns the count from the dynamic array
        return 0;       // OP_CONSTANT has an operand, and that operand is the index into the chunk
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}


// I just spent 30 minutes looking for ways to reorder my code, when really I just 
// needed to put the function prototypes at the correct location
// which would have been told to me if I had read ahead just 1 code block.
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);



static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence( (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:        emitByte(OP_ADD);           break;
        case TOKEN_MINUS:       emitByte(OP_SUBTRACT);      break;
        case TOKEN_STAR:        emitByte(OP_MULTIPLY);      break;
        case TOKEN_SLASH:       emitByte(OP_DIVIDE);        break;
        default: return;    // Unreachable
    }


}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("[prefix parsing] Expect expression.");
        return;
    }
    prefixRule();
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }


}



// this was the hardest part of jayko
// should provide pratt parsing logic for the folloiwng four things:
// Number Literals: 123
// Parentheses for grouping (123)
// Unary negation: -123
// +,-,*,/
static void expression(){
    parsePrecedence(PREC_ASSIGNMENT);
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
    double value = strtod(parser.previous.start, NULL); // we only accept floats rn lmao
    emitConstant(NUMBER_VAL(value));
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    // compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

// hopefully the gods forgive me for copy pasting this from the book 
// instead of manually typing it like I have been doing with the rest.
// This is similar to how "typeclasses" would have implemented it.
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE}, // nud, led, rbp
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};



static ParseRule* getRule(TokenType type) {
    return &rules[type];
}


bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    
    advance();          // "primes the pump" what
    expression();       // i am intimidated by this chapter because pratt parsing is hard!
    //consume(TOKEN_EOF, "EXPECT END OF EXPRESSION"); // even tho parsing an compiling are used interchangeably in this book IDT that means they are necessarily the same thing
    endCompiler();
    return !parser.hadError;



    //int line = -1;
    //for (;;) {
    //    Token token = scanToken();
    //    if (token.line != line) {
    //        printf("%4d ", token.line);
    //        line = token.line;
    //    } else {
    //        printf("   | ");
    //    }
    //    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    //    if (token.type == TOKEN_EOF) break;
    //}
}


