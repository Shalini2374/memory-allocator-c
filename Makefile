CC = gcc
CFLAGS = -Wall -Wextra -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lrt

all: main stress multi

main: allocator.c main.c
	$(CC) $(CFLAGS) allocator.c main.c -o main $(LDFLAGS)

stress: allocator.c stress_test.c
	$(CC) $(CFLAGS) allocator.c stress_test.c -o stress $(LDFLAGS)

multi: allocator.c multi_thread.c
	$(CC) $(CFLAGS) allocator.c multi_thread.c -o multi $(LDFLAGS)

clean:
	rm -f main stress multi my_allocator