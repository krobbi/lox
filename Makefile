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
CLOX := $(BIN_DIR)/clox

# Utility scripts:
UTILS_DIR := utils
MERGE := $(UTILS_DIR)/merge.lox
COMPARE := $(UTILS_DIR)/compare.lox

# Lox standard library:
STD_DIR := std
STD_SRCS := $(wildcard $(STD_DIR)/*.lox)

# Lynx:
LYNX_DIR := lynx
LYNX_SRCS := $(call wild,$(LYNX_DIR),*.lox)
LYNX_MAIN := $(LYNX_DIR)/main.lox
LYNX_MRGE := $(LYNX_DIR)/merge.txt
LYNX_STG0 := $(BIN_DIR)/lynx_stage_0.lox
LYNX_STG1 := $(BIN_DIR)/lynx_stage_1.lox
LYNX_STG2 := $(BIN_DIR)/lynx_stage_2.lox
LYNX := $(BIN_DIR)/lynx.lox

# Windows executables:
ifeq ($(OS),Windows_NT)
	CLOX := $(CLOX).exe
endif

# Build Lynx:
.PHONY: all
all: $(LYNX)

# Clean binaries directory:
.PHONY: clean
clean:
	@ echo "Cleaning '$(BIN_DIR)/'..." 1>&2
	@ rm -rf -- $(BIN_DIR)

# Make binaries directory:
$(BIN_DIR):
	@ echo "Making '$@/'..." 1>&2
	@ mkdir $@

# Compile Clox objects from Clox sources:
$(BIN_DIR)/clox_%.o: $(CLOX_DIR)/%.c $(CLOX_HDRS) | $(BIN_DIR)
	@ echo "Compiling '$@'..." 1>&2
	@ $(CC) $(CFLAGS) -c $< -o $@

# Link Clox executable from Clox objects:
$(CLOX): $(CLOX_OBJS)
	@ echo "Linking '$@'..." 1>&2
	@ $(CC) $(CFLAGS) $^ -o $@

# Merge Lynx stage 0 from Lynx sources:
.DELETE_ON_ERROR: $(LYNX_STG0)
$(LYNX_STG0): $(LYNX_MRGE) $(LYNX_SRCS) $(STD_SRCS) $(CLOX) $(MERGE)
	@ echo "Merging '$@'..." 1>&2
	@ $(CLOX) $(MERGE) $< $(STD_DIR) $@

# Preprocess Lynx stage 1 from Lynx stage 0:
.DELETE_ON_ERROR: $(LYNX_STG1)
$(LYNX_STG1): $(LYNX_STG0)
	@ echo "Preprocessing '$@'..." 1>&2
	@ $(CLOX) $< --std $(STD_DIR) --output $@ -- $(LYNX_MAIN)

# Preprocess Lynx stage 2 from Lynx stage 1:
.DELETE_ON_ERROR: $(LYNX_STG2)
$(LYNX_STG2): $(LYNX_STG1)
	@ echo "Preprocessing '$@'..." 1>&2
	@ $(CLOX) $< --std $(STD_DIR) --output $@ -- $(LYNX_MAIN)

# Preprocess Lynx from Lynx stage 2:
.DELETE_ON_ERROR: $(LYNX)
$(LYNX): $(LYNX_STG2) $(COMPARE)
	@ echo "Preprocessing '$@'..." 1>&2
	@ $(CLOX) $< --std $(STD_DIR) --output $@ -- $(LYNX_MAIN)
	@ echo "Comparing '$@' to '$<'..."
	@ $(CLOX) $(COMPARE) $@ $<
