#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.h"

// Compile source code to a chunk and return whether no errors occurred.
bool compile(const char* source, Chunk *chunk);

#endif // !clox_compiler_h
