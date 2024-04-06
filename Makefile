CC = gcc
LD = gcc

CCFLAGS = -xc -fdiagnostics-color=always -Wall -Winvalid-pch -Wextra -Wpedantic -std=c2x
CCFLAGS_DEBUG = -g -fsanitize=address,undefined
CCFLAGS_RELEASE = -flto=auto -O3 -march=haswell

SRC = $(wildcard src/*.c)
BIN = build
OUT = main
DEP = $(patsubst src/%,$(BIN)/%.d,$(SRC))

DEBUG_BIN = $(BIN)/debug
DEBUG_OBJ = $(patsubst src/%.c,$(DEBUG_BIN)/%.c.o,$(SRC))
DEBUG_OUT = $(DEBUG_BIN)/$(OUT)

RELEASE_BIN = $(BIN)/release
RELEASE_OBJ = $(patsubst src/%.c,$(RELEASE_BIN)/%.c.o,$(SRC))
RELEASE_OUT = $(RELEASE_BIN)/$(OUT)

-include $(DEP)

default: run_debug

debug: $(DEBUG_OUT)
release: $(RELEASE_OUT)

$(BIN):
	mkdir $(BIN)

$(DEBUG_BIN):
	mkdir -p $(DEBUG_BIN)

$(RELEASE_BIN):
	mkdir -p $(RELEASE_BIN)

$(DEBUG_OBJ): $(DEBUG_BIN)/%.c.o: src/%.c
	$(CC) -o $@ -c $(CCFLAGS) $(CCFLAGS_DEBUG) $<

$(RELEASE_OBJ): $(RELEASE_BIN)/%.c.o: src/%.c
	$(CC) -o $@ -c $(CCFLAGS) $(CCFLAGS_RELEASE) $<

$(DEBUG_OUT): $(DEBUG_BIN) $(DEBUG_OBJ)
	$(LD) $(CCFLAGS_DEBUG) -o $(DEBUG_BIN)/$(OUT) $(DEBUG_OBJ)

$(RELEASE_OUT): $(RELEASE_BIN) $(RELEASE_OBJ)
	$(LD) $(CCFLAGS_RELEASE) -o $(RELEASE_BIN)/$(OUT) $(RELEASE_OBJ)

run_debug: $(DEBUG_OUT)
	$(DEBUG_OUT)

run_release: $(RELEASE_BIN)/$(OUT)
	$(RELEASE_BIN)/$(OUT)

clean:
	rm -r $(BIN)
