CC ?= gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Wpedantic -Ic_harness
SAN := -fsanitize=address,undefined

.PHONY: all test clean

all: build/test_hal_invariants

build/test_hal_invariants: c_harness/stm32_hal_invariants.c c_harness/test_hal_invariants.c c_harness/stm32_hal_invariants.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SAN) c_harness/stm32_hal_invariants.c c_harness/test_hal_invariants.c -o build/test_hal_invariants

test: all
	./build/test_hal_invariants

clean:
	rm -rf build
