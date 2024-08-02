IDIR=.
CC=gcc
CFLAGS=-I$(IDIR) -g -lm

BUILD_DIR=build
LIBS=

_DEPS=lexer.h log_error.h chunk.h value.h memory.h vm.h stack.h parser.h object.h hash_map.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=lexer.o log_error.o chunk.o value.o memory.o vm.o stack.o parser.o object.o hash_map.o
OBJ=$(patsubst %,$(BUILD_DIR)/%,$(_OBJ))

EXEC_NAME=interpreter

$(BUILD_DIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -o $(EXEC_NAME) $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*.o  
