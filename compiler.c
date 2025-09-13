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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;             // the nud()
    ParseFn infix;              // the led()
    Precedence precedence;      // the right binding power
} ParseRule;

typdef struct {
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;


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

static bool check(TokenType type) {
    // check if the current token has the given type
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
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
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);



static void binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence( (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:     emitBytes(OP_EQUAL, OP_NOT);     break;
        case TOKEN_EQUAL_EQUAL:    emitByte(OP_EQUAL);              break;
        case TOKEN_GREATER:        emitByte(OP_GREATER);           break;
        case TOKEN_GREATER_EQUAL:  emitBytes(OP_LESS, OP_NOT);      break;
        case TOKEN_LESS:           emitByte(OP_LESS);               break;
        case TOKEN_LESS_EQUAL:     emitBytes(OP_GREATER, OP_NOT);   break;

        case TOKEN_PLUS:           emitByte(OP_ADD);                break;
        case TOKEN_MINUS:          emitByte(OP_SUBTRACT);           break;
        case TOKEN_STAR:           emitByte(OP_MULTIPLY);           break;
        case TOKEN_SLASH:          emitByte(OP_DIVIDE);             break;

        default: return;    // Unreachable
    }


}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // Unreachable
    }
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("[prefix parsing] Expect expression.");
        return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("invalid assignment target.");
    }


}

static uint8_t identifierConstant(Token* name) {
    return makeConstant( OBJ_VAL(copyString(name-> start, name->length)));
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
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


static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);

}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}


static void synchronize() {
    parser.panicMode = false;

    while(parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ; // Do nothing

        }

        advance();
    }
}

// in lox, declarations are something even bigger than statements()
static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else {
        expressionStatement();
    }

}


static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL); // we only accept floats rn lmao
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
    // +1 and -2 trim the quotation marks, its -2 because a string will have "\0. 
    // We would translate \n here fi lox supported escape sequences
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, 
                                    parser.previous.length -2)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t arg = identifierConstant(&name);
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_GLOBAL, arg);
    } else {
        emitBytes(OP_GET_GLOBAL, arg);
    }
}
static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction
    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // Unreachable.
    }
}

// hopefully the gods forgive me for copy pasting this from the book 
// instead of manually typing it like I have been doing with the rest.
// This is similar to how "typeclasses" would have implemented it.
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping,    NULL,   PREC_NONE}, // nud, led, rbp
  [TOKEN_RIGHT_PAREN]   = {NULL,        NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,        NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,        NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,        NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,       binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,        binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,        NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,        binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,        binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,       NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,        binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,        binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,        binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,        binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,        binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,        binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable,        NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,        NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,      NULL,   PREC_NONE},

  [TOKEN_AND]           = {NULL,        NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,        NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,     NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,        NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,        NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,        NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,        NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,        NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,        NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,     NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,        NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,        NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,        NULL,   PREC_NONE},
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

    while (!match(TOKEN_EOF)) {
        declaration();
    }


    //expression();       // i am intimidated by this chapter because pratt parsing is hard!
    //consume(TOKEN_EOF, "EXPECT END OF EXPRESSION"); // even tho parsing an compiling are used interchangeably in this book IDT that means they are necessarily the same thing
    endCompiler();
    return !parser.hadError;
}


