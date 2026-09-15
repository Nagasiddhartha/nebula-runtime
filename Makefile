CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -std=c11 -D_GNU_SOURCE
TARGET = build/nebula

SRCS = $(shell find src -name '*.c')
OBJS = $(SRCS:src/%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean all
	./$(TARGET)

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --suppressions=valgrind.supp --max-stackframe=2000000000 ./$(TARGET)

bench:
	@mkdir -p build
	$(CC) $(CFLAGS) benchmarks/bench_alloc.c src/allocator/allocator_core.c src/allocator/policy_first_fit.c src/allocator/policy_best_fit.c src/allocator/policy_buddy.c src/allocator/policy_slab.c -o build/bench
	./build/bench

.PHONY: all clean asan valgrind bench
