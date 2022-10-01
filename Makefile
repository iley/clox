CC:=clang
# -DDEBUG_PRINT_CODE -DDEBUG_STRESS_GC -DDEBUG_LOG_GC -DDEBUG_TRACE_EXECUTION 
DEFINES:=-DDEBUG_PRINT_CODE
CFLAGS:=-std=c17 -Wall -Wextra -Werror -O1 -g -fsanitize=address -fno-omit-frame-pointer $(DEFINES)

SRCS=chunk.c compiler.c debug.c memory.c object.c scanner.c table.c value.c vm.c
OBJS=$(SRCS:.c=.o)

.PHONY: clean

clox: main.c $(OBJS)
	$(CC) $(CFLAGS) -o clox $^

$(OBJS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f clox $(OBJS)
	rm -rf clox.dSYM
