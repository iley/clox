CC:=clang
CFLAGS:=-std=c17 -Wall -Wextra -Werror -DDEBUG_TRACE_EXECUTION

SRCS=chunk.c compiler.c debug.c memory.c scanner.c value.c vm.c
OBJS=$(SRCS:.c=.o)

.PHONY: clean

clox: main.c $(OBJS)
	$(CC) $(CFLAGS) -o clox $^

$(OBJCS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f clox $(OBJS)
