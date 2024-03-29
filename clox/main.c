#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

#ifdef EXTENSIONS
#include "extension.h"
#endif // EXTENSIONS

// Read and interpret user input in a loop.
static void repl() {
	char line[1024];
	
	for (;;) {
		printf("> ");
		
		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}
		
		interpret(line);
	}
}

// Read a source file from a path.
static char *readFile(const char *path) {
	FILE *file = fopen(path, "rb");
	
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(74);
	}
	
	fseek(file, 0L, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);
	
	char *buffer = (char*)malloc(fileSize + 1);
	
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(74);
	}
	
	size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
	
	if (bytesRead < fileSize) {
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(74);
	}
	
	buffer[bytesRead] = '\0';
	
	fclose(file);
	return buffer;
}

// Read and interpret a source file from a path.
static void runFile(const char *path) {
	char *source = readFile(path);
	InterpretResult result = interpret(source);
	free(source);
	
	if (result == INTERPRET_COMPILE_ERROR) {
		exit(65);
	}
	
	if (result == INTERPRET_RUNTIME_ERROR) {
		exit(70);
	}
}

// Interpret a source file from arguments, or run a REPL.
int main(int argc, const char *argv[]) {
#ifdef EXTENSIONS
	initExtensions(argc, argv);
#endif // EXTENSIONS
	
	initVM();
	
	if (argc == 1) {
		repl();
#ifdef EXTENSIONS
	} else if (argc > 1) {
#else // EXTENSIONS
	} else if (argc == 2) {
#endif // !EXTENSIONS
		runFile(argv[1]);
	} else {
#ifdef EXTENSIONS
		fprintf(stderr, "Usage: clox [<path> [<args>...]]\n");
#else // EXTENSIONS
		fprintf(stderr, "Usage: clox [path]\n");
#endif // !EXTENSIONS
		
		exit(64);
	}
	
	freeVM();
	
#ifdef EXTENSIONS
	freeExtensions();
#endif // EXTENSIONS
	
	return 0;
}
