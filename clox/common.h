#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Disassemble bytecode after compilation.
//#define DEBUG_PRINT_CODE

// Disassemble instructions as they are interpreted.
//#define DEBUG_TRACE_EXECUTION

// The number of unique 8-bit unsigned integers.
#define UINT8_COUNT (UINT8_MAX + 1)

#endif // !clox_common_h
