#!/usr/bin/zsh
gcc -std=c17 -g chunk.c main.c memory.c debug.c value.c vm.c compiler.c scanner.c object.c table.c -o clox
