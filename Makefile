# Makefile for BAZOOKI OS - Phase 1 Motorcycle Dashboard
# Requires: pthread, C99

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I include -pthread
LDFLAGS = -pthread -lm

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN = bazooki_os

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

all: $(BUILD_DIR) $(BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN)

run: $(BIN)
	./$(BIN)
