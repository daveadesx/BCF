CC = gcc
CFLAGS = -Wall -Werror -Wextra -pedantic -std=c99 -g
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = betty-fmt

# Test objects (exclude main.o)
TEST_OBJS = $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_DIR)/test_lexer

# Unit tests
$(TEST_DIR)/test_lexer: $(TEST_DIR)/test_lexer.c $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test-unit: $(TEST_DIR)/test_lexer
	@echo "Running unit tests..."
	@$(TEST_DIR)/test_lexer

# Integration tests
test-integration: $(TARGET)
	@echo "Running integration tests..."
	@./tests/run_tests.sh

# Run all tests
test: test-unit test-integration

.PHONY: all clean test test-unit test-integration
