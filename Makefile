CC ?= clang
CFLAGS ?= -std=c2x -O3 -Wall -Wextra -Iinclude -march=native -flto
AR ?= ar

TARGET_LIB = libring_io.a
SRCS = src/ring_io.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET_LIB) bench test

$(TARGET_LIB): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bench: benchmarks/bench_iouring_vs_epoll.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -L. -lring_io -o bin_bench

test: tests/test_submission_queue.c $(TARGET_LIB)
	$(CC) $(CFLAGS) $< -L. -lring_io -o bin_test

run-test: test
	./bin_test

run-bench: bench
	./bin_bench

clean:
	rm -f $(OBJS) $(TARGET_LIB) bin_bench bin_test
