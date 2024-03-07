#ifndef clox_extension_h
#define clox_extension_h

#include "common.h"
#include "object.h"

// A function for defining a native function.
typedef void (*DefineNativeFn)(const char *name, NativeFn function);

// Initialize extension data from command line arguments.
void initExtensions(int argc, const char *argv[]);

// Define native extension functions using a native definition function.
void defineExtensions(DefineNativeFn defineNative);

// Free extension data.
void freeExtensions();

#endif // !clox_extension_h
