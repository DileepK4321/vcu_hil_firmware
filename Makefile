CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinc -Isrc

SRC_APP = src/vcu_app.c
SRC_STATE = src/vcu_app.c src/vcu_state.c
SRC_DERATE = src/vcu_derate.c

TARGET_APPS = test_apps_safety
TARGET_STATE = test_state_machine
TARGET_DERATE = test_derate

all: $(TARGET_APPS) $(TARGET_STATE) $(TARGET_DERATE)
	@echo "--- Executing APPS Unit Tests ---"
	./$(TARGET_APPS)
	@echo "--- Executing State Machine Unit Tests ---"
	./$(TARGET_STATE)
	@echo "--- Executing Derating Engine Unit Tests ---"
	./$(TARGET_DERATE)

$(TARGET_APPS): $(SRC_APP) test/test_apps_safety.c
	$(CC) $(CFLAGS) $(SRC_APP) test/test_apps_safety.c -o $(TARGET_APPS)

$(TARGET_STATE): $(SRC_STATE) test/test_state_machine.c
	$(CC) $(CFLAGS) $(SRC_STATE) test/test_state_machine.c -o $(TARGET_STATE)

$(TARGET_DERATE): $(SRC_DERATE) test/test_derate.c
	$(CC) $(CFLAGS) $(SRC_DERATE) test/test_derate.c -o $(TARGET_DERATE)

clean:
	rm -f $(TARGET_APPS) $(TARGET_STATE) $(TARGET_DERATE)

.PHONY: all clean