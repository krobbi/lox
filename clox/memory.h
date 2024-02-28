#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

// Allocate a static array from a capacity.
#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

// Free a pointer from its type.
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// Get the next capacity from a dynamic array's capacity.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// Reallocate a dynamic array from an old capacity to a new capacity.
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
	(type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

// Free a dynamic array from an old capacity.
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

// Reallocate a block of memory from an old size to a new size.
void *reallocate(void *pointer, size_t oldSize, size_t newSize);

// Free all allocated objects.
void freeObjects();

#endif // !clox_memory_h
