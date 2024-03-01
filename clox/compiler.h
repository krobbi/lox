#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

// Compile a function object from source code.
ObjFunction *compile(const char *source);

#endif // !clox_compiler_h
