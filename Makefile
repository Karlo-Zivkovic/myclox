# Compiler settings
CC = gcc
CFLAGS = -fsanitize=address -g -Wall -Wextra

# File settings
SRC = $(wildcard *.c)
TARGET = main

# Default target
all: $(TARGET)

# Compile directly to executable without intermediate .o files
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) "test.lox" 

# Clean
clean:
	rm -f $(TARGET) *.o
	@echo "Cleaned all files"

.PHONY: clean all
