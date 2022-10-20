DEBUG_MODE:=

CC:=clang
DEFINES:=
# -DDEBUG_PRINT_CODE -DDEBUG_STRESS_GC -DDEBUG_LOG_GC -DDEBUG_TRACE_EXECUTION 

ifeq ($(DEBUG_MODE),)
# Release mode C flags.
MODE_CFLAGS:=-O2
else
# Debug mode C flags.
MODE_CFLAGS:=-O0 -fsanitize=address -fno-omit-frame-pointer -g
endif

CFLAGS:=-std=c17 -Wall -Wextra -Werror $(DEFINES) $(MODE_CFLAGS)

SRCS=chunk.c compiler.c debug.c memory.c object.c scanner.c table.c value.c vm.c
OBJS=$(SRCS:.c=.o)

.PHONY: clean

all: clox

clox: main.c $(OBJS)
	$(CC) $(CFLAGS) -o clox $^

$(OBJS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f clox $(OBJS)
	rm -rf clox.dSYM
