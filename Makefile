CC:=clang
CFLAGS:=-std=c17 -Wall -Wextra -Werror

SRCS=chunk.c memory.c
OBJS=$(SRCS:.c=.o)

.PHONY: clean

clox: main.c $(OBJS)
	$(CC) $(CFLAGS) -o clox $^

$(OBJCS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f clox $(OBJS)
