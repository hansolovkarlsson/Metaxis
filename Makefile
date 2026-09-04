# Prototype -- a C11 compiler and make, and nothing else.

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wno-unused-parameter
CPPFLAGS = -Iinclude -D_POSIX_C_SOURCE=200809L

SRC  = src/util.c src/header.c src/lex.c src/expand.c
OBJ  = $(SRC:src/%.c=build/%.o) build/pt.o
BIN  = bin/pt

EXAMPLES = $(wildcard examples/*.pt)
OUTS     = $(EXAMPLES:examples/%.pt=examples/%.out)

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(OBJ)

build/%.o: src/%.c include/pt.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

build/pt.o: cmd/pt.c include/pt.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Re-record every example's output. Read the diff before committing it.
record: $(BIN)
	@for f in $(EXAMPLES); do \
	    ./$(BIN) -o $${f%.pt}.out $$f || exit 1; \
	    echo "recorded $${f%.pt}.out"; \
	done

# Every example still expands to what is recorded beside it.
check: $(BIN)
	@fail=0; \
	for f in $(EXAMPLES); do \
	    want=$${f%.pt}.out; \
	    if [ ! -f $$want ]; then echo "MISSING $$want"; fail=1; continue; fi; \
	    if ./$(BIN) $$f 2>build/err.txt | diff -u $$want - > build/diff.txt; then \
	        echo "ok      $$f"; \
	    else \
	        echo "FAILED  $$f"; cat build/err.txt build/diff.txt; fail=1; \
	    fi; \
	done; \
	sh tests/errors.sh ./$(BIN) || fail=1; \
	exit $$fail

test: check

clean:
	rm -rf build bin

.PHONY: all check test record clean
