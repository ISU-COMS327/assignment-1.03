CC=gcc
TARGET=generate_dungeon

$(TARGET): $(TARGET).c
	@gcc $(TARGET).c -o $(TARGET) -Wall -Werror -ggdb
	@echo "Made $(TARGET)"

.PHONY: clean
clean:
	@rm -rf $(TARGET) *.o *.dSYM
	@echo "Directory cleaned."
