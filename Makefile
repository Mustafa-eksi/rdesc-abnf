.PHONY: default
CFLAGS=-Wall -Wextra -Wshadow -pedantic -std=c99 -ggdb
# Set the path to your librdesc source folder. (REQUIRED)
RDESC_DIR := vendor/rdesc

# Optionally configure the build variables
RDESC_MODE := debug
RDESC_FEATURES := stack dump_cst dump_bnf

default: main

# You may prefer installing librdesc using git
$(RDESC_DIR)/rdesc.mk:
	git clone https://github.com/metwse/rdesc.git $(RDESC_DIR)

# Include the librdesc build system
include $(RDESC_DIR)/rdesc.mk

main: src/main.c src/lexer.c src/grammar.h $(RDESC)
	$(CC) $(CFLAGS) -I$(RDESC_INCLUDE_DIR) $< $(RDESC) -o $@

test: tests/adder_test.c main
	$(CC) $(CFLAGS) -I$(RDESC_INCLUDE_DIR) $< $(RDESC) -o $@

clean:
	rm main
	rm test
