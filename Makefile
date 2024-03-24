# C compiler:
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Werror -O3 -flto

# Recursive wildcard:
# From https://blog.jgc.org/2011/07/gnu-make-recursive-wildcard-function.html
wild = $(foreach d,$(wildcard $1*),$(call wild,$d/,$2)$(filter $(subst *,%,$2),$d))

# Binaries:
BIN_DIR := bin

# Clox:
CLOX_DIR := clox
CLOX_SRCS := $(wildcard $(CLOX_DIR)/*.c)
CLOX_HDRS := $(wildcard $(CLOX_DIR)/*.h)
CLOX_OBJS := $(CLOX_SRCS:$(CLOX_DIR)/%.c=$(BIN_DIR)/clox_%.o)
CLOX_EXEC := $(BIN_DIR)/clox

# Lox Merge:
MERGE := merge/merge.lox

# Lox standard library:
STD_DIR := std
STD_SRCS := $(call wild,$(STD_DIR),*.lox)

# Lynx:
LYNX_DIR := lynx
LYNX_SRCS := $(call wild,$(LYNX_DIR),*.lox)
LYNX_MRGE := $(LYNX_DIR)/merge.txt
LYNX_STG0 := $(BIN_DIR)/lynx_stage_0.lox
LYNX_STG1 := $(BIN_DIR)/lynx_stage_1.lox

# Windows executables:
ifeq ($(OS),Windows_NT)
	CLOX_EXEC := $(CLOX_EXEC).exe
endif

# Build Lynx:
.PHONY: all
all: $(LYNX_STG1)

# Clean binaries directory:
.PHONY: clean
clean:
	@ echo "Cleaning '$(BIN_DIR)/'..."
	@ rm -rf -- $(BIN_DIR)

# Make binaries directory:
$(BIN_DIR):
	@ echo "Making '$@/'..."
	@ mkdir $@

# Compile Clox objects from Clox sources:
$(BIN_DIR)/clox_%.o: $(CLOX_DIR)/%.c $(CLOX_HDRS) | $(BIN_DIR)
	@ echo "Compiling '$<'..."
	@ $(CC) $(CFLAGS) -c $< -o $@

# Link Clox executable from Clox objects:
$(CLOX_EXEC): $(CLOX_OBJS)
	@ echo "Linking '$@'..."
	@ $(CC) $(CFLAGS) $^ -o $@

# Merge Lynx stage 0 from Lynx sources:
.DELETE_ON_ERROR: $(LYNX_STG0)
$(LYNX_STG0): $(CLOX_EXEC) $(MERGE) $(LYNX_MRGE) $(LYNX_SRCS) $(STD_SRCS)
	@ echo "Merging '$@'..."
	@ $(CLOX_EXEC) $(MERGE) $(LYNX_MRGE) $(STD_DIR) $@

# Preprocess Lynx stage 1 from Lynx stage 0:
.DELETE_ON_ERROR: $(LYNX_STG1)
$(LYNX_STG1): $(LYNX_STG0)
	@ echo "Preprocessing '$@'..."
	@ $(CLOX_EXEC) $<
