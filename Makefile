# Metaxis -- a C11 compiler and make, and nothing else.

CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wno-unused-parameter
CPPFLAGS = -Imetaxis/include -D_POSIX_C_SOURCE=200809L

SRC  = metaxis/src/util.c metaxis/src/header.c metaxis/src/lex.c \
       metaxis/src/expand.c metaxis/src/code.c
OBJ  = $(SRC:metaxis/src/%.c=build/%.o) build/mx.o
BIN  = bin/mx

EXAMPLES = $(wildcard examples/*.mx)
OUTS     = $(EXAMPLES:examples/%.mx=examples/%.out)

# Seconds any one expansion gets before it is killed. Every example here runs in
# milliseconds, so this is not a performance budget -- it is the only way the
# suite can report *did not terminate*, which no recorded .out can express. See
# tests/limit.sh. Raise it on the command line if a machine is loaded:
# `make check LIMIT=30`.
LIMIT ?= 10

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(OBJ)

build/%.o: metaxis/src/%.c metaxis/include/mx.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

build/mx.o: metaxis/cmd/mx.c metaxis/include/mx.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# Re-record every example's output. Read the diff before committing it.
record: $(BIN)
	@for f in $(EXAMPLES); do \
	    for b in $$(sh tests/limit.sh $(LIMIT) ./$(BIN) -g $$f | awk '/^backend/{print $$2}'); do \
	        sh tests/limit.sh $(LIMIT) ./$(BIN) -b $$b -o $${f%.mx}-$$b.out $$f || exit 1; \
	        echo "recorded $${f%.mx}-$$b.out"; \
	    done; \
	    sh tests/limit.sh $(LIMIT) ./$(BIN) -o $${f%.mx}.out $$f; \
	    rc=$$?; \
	    if [ $$rc -eq 124 ]; then \
	        echo "FAILED  $$f did not finish in $(LIMIT)s -- nothing recorded"; \
	        exit 1; \
	    fi; \
	    [ $$rc -eq 0 ] || exit 1; \
	    echo "recorded $${f%.mx}.out"; \
	done

# Every example still expands to what is recorded beside it.
check: $(BIN)
	@fail=0; \
	for f in $(EXAMPLES); do \
	    want=$${f%.mx}.out; \
	    if [ ! -f $$want ]; then echo "MISSING $$want"; fail=1; continue; fi; \
	    sh tests/limit.sh $(LIMIT) ./$(BIN) $$f >build/got.txt 2>build/err.txt; \
	    rc=$$?; \
	    if [ $$rc -eq 124 ]; then \
	        echo "FAILED  $$f: did not finish in $(LIMIT)s, and was killed."; \
	        echo "        A hang is the one failure a recorded .out cannot show,"; \
	        echo "        so it is reported here rather than waited on."; \
	        fail=1; continue; \
	    fi; \
	    if [ $$rc -ne 0 ]; then \
	        echo "FAILED  $$f"; cat build/err.txt; fail=1; continue; \
	    fi; \
	    if diff -u $$want build/got.txt > build/diff.txt; then \
	        echo "ok      $$f"; \
	    else \
	        echo "FAILED  $$f"; cat build/err.txt build/diff.txt; fail=1; \
	    fi; \
	    for b in $$(sh tests/limit.sh $(LIMIT) ./$(BIN) -g $$f | awk '/^backend/{print $$2}'); do \
	        want=$${f%.mx}-$$b.out; \
	        if [ ! -f $$want ]; then echo "MISSING $$want"; fail=1; continue; fi; \
	        sh tests/limit.sh $(LIMIT) ./$(BIN) -b $$b $$f >build/got.txt 2>build/err.txt; \
	        rc=$$?; \
	        if [ $$rc -ne 0 ]; then \
	            echo "FAILED  $$f -b $$b"; cat build/err.txt; fail=1; continue; \
	        fi; \
	        if diff -u $$want build/got.txt > build/diff.txt; then \
	            echo "ok      $$f -b $$b"; \
	        else \
	            echo "FAILED  $$f -b $$b"; cat build/err.txt build/diff.txt; fail=1; \
	        fi; \
	    done; \
	done; \
	LIMIT=$(LIMIT) sh tests/errors.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/hygiene.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/docs.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/pascal.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/basic.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/asm.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/python.sh ./$(BIN) || fail=1; \
	LIMIT=$(LIMIT) sh tests/scale.sh ./$(BIN) || fail=1; \
	exit $$fail

test: check

clean:
	rm -rf build bin

.PHONY: all check test record clean
