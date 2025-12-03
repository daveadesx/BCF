CC = gcc
CFLAGS = -Wall -Werror -Wextra -pedantic -std=c99 -g -I include
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Only compile the needed source files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/formatter.c $(SRC_DIR)/utils.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = betty-fmt

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
