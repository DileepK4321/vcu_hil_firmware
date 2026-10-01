CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinc -Isrc

SRC = src/vcu_app.c test/test_apps_safety.c
TARGET = sil_test_runner

all: $(TARGET)
	./$(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean