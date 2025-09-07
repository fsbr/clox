#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
    char line[1024];                                // form a character buffer of 1024 units long
    for (;;) {                                      // loop forever
        printf("> ");                               // terminal prompt

        if (!fgets(line, sizeof(line), stdin)) {    // reads from stdin , this is smart. fgets runs no matter what
            printf("\n");                           // print a new line
            break;                                  // break out of loop
        }

        interpret(line);                            // use the interpreter on the line. IN THE REPL
    }
}

static char* readFile(const char* path) {                               // function to read a file
    FILE* file = fopen(path, "rb");                                     // read-only as binary

    if (file == NULL) {                                                 // file not found
        fprintf(stderr, "Could not open file \"%s\".\n", path);         // error message
        exit(74);                                                       // error code 74 is a failure in readFile
    }

    fseek(file, 0L, SEEK_END);                                          // run the file pointer throught he whole file
    size_t fileSize = ftell(file);                                      // report the filesize
    rewind(file);                                                       // put the file pointer back to the beginning 

    char* buffer = (char *)malloc(fileSize + 1);                        // preparation to add a '\0' to the end of the source code
    if (buffer == NULL) {                                               // malloc could faile
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);   // error message
        exit(74);                                                       // error code 74 is a failure in readFile
    }



    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);     // read the file into a variable called byteRead
    if (bytesRead < fileSize) {                                         // the thing we are reading should be the same size as the file we just pointed at
        fprintf(stderr, "Could not read file \"%s\".\n", path);         // error message based on filesize
        exit(74);                                                       // exit 74
    }

    buffer[bytesRead] = '\0';                                           // add our own null character

    fclose(file);                                                       // close the file
    return buffer;                                                      // return a pointer to the head of the file
}

static void runFile(const char* path) {                                 // runfile for compiled mode 
    char* source = readFile(path);                                      // read the file from the path
    InterpretResult result = interpret(source);                         // run the interpreter over the entire file
    free(source);                                                       // FREE the allocated memory

    if (result == INTERPRET_COMPILE_ERROR) exit(65);                    // compilation error code 65
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);                    // runtime error code 70
}

int main(int argc, const char* argv[]) {                                // main and read from 
    initVM();                                                           // initialize our VM
    
    if (argc == 1) {                                                    // since argc always include the file name, if we pass no arguments, enter the repl
        repl();
    } else if (argc == 2) {                                             // if theres a file run it
        runFile(argv[1]); 
    } else {
        fprintf(stderr, "Usage: clox [path]\n");                        // help for using the compiler
        exit(64);                                                       // exitcode 64 for help
    }

    freeVM();                                                           // free the VM
    return 0;                                                           // Successful exit!
}
