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

Parser parser;                                  // remember for a "real" language this is bad. 

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

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);

    parser.hadError = false;
    parser.panicMode = false;
    
    advance();          // "primes the pump" what
    expression();       // i am intimidated by this chapter because pratt parsing is hard!
    consume(TOKEN_EOF, "EXPECT END OF EXPRESSION"); // even tho parsing an compiling are used interchangeably in this book IDT that means they are necessarily the same thing
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


