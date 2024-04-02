#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Use a smaller value representation.
#define NAN_BOXING

// Use non-standard native extension functions.
#define EXTENSIONS

// Merge constant indices for constants with equal values.
#define CONSTANT_MERGING

// Disassemble bytecode after compilation.
#define DEBUG_PRINT_CODE

// Disassemble instructions as they are interpreted.
//#define DEBUG_TRACE_EXECUTION

// Run the garbage collector before every allocation.
//#define DEBUG_STRESS_GC

// Log garbage collection information.
//#define DEBUG_LOG_GC

// The number of unique 8-bit unsigned integers.
#define UINT8_COUNT (UINT8_MAX + 1)

#endif // !clox_common_h
