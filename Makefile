# C compiler:
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Werror -O3 -flto

# Binaries:
BIN_DIR := bin

# Clox:
CLOX_DIR := clox
CLOX_SRCS := $(wildcard $(CLOX_DIR)/*.c)
CLOX_HDRS := $(wildcard $(CLOX_DIR)/*.h)
CLOX_OBJS := $(CLOX_SRCS:$(CLOX_DIR)/%.c=$(BIN_DIR)/clox_%.o)
CLOX_EXEC := $(BIN_DIR)/clox

# Windows executables:
ifeq ($(OS),Windows_NT)
	CLOX_EXEC := $(CLOX_EXEC).exe
endif

# Build Clox:
.PHONY: all
all: $(CLOX_EXEC)

# Build and run Clox:
.PHONY: run
run: $(CLOX_EXEC)
	@ echo "Running '$<'..."
	@ $(CLOX_EXEC) test/args.lox foo bar baz

# Clean binaries directory:
.PHONY: clean
clean:
	@ echo "Cleaning '$(BIN_DIR)/'..."
	@ rm -rf -- $(BIN_DIR)

# Make binaries directory:
$(BIN_DIR):
	@ echo "Making '$@/'..."
	@ mkdir $(BIN_DIR)

# Compile Clox objects from Clox sources.
$(BIN_DIR)/clox_%.o: $(CLOX_DIR)/%.c $(CLOX_HDRS) | $(BIN_DIR)
	@ echo "Compiling '$<'..."
	@ $(CC) $(CFLAGS) -c $< -o $@

# Link Clox executable from Clox objects.
$(CLOX_EXEC): $(CLOX_OBJS)
	@ echo "Linking '$@'..."
	@ $(CC) $(CFLAGS) $^ -o $@
