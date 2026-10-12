CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinc -Isrc

SRC_APP = src/vcu_app.c
SRC_STATE = src/vcu_app.c src/vcu_state.c src/vcu_derate.c  # <-- Added src/vcu_derate.c here
SRC_DERATE = src/vcu_derate.c
SRC_CAN = src/vcu_can.c

TARGET_APPS = test_apps_safety
TARGET_STATE = test_state_machine
TARGET_DERATE = test_derate
TARGET_CAN = test_can

all: $(TARGET_APPS) $(TARGET_STATE) $(TARGET_DERATE) $(TARGET_CAN)
	@echo "--- Executing APPS Unit Tests ---"
	./$(TARGET_APPS)
	@echo "--- Executing State Machine Unit Tests ---"
	./$(TARGET_STATE)
	@echo "--- Executing Derating Engine Unit Tests ---"
	./$(TARGET_DERATE)
	@echo "--- Executing CAN Middleware Unit Tests ---"
	./$(TARGET_CAN)

$(TARGET_APPS): $(SRC_APP) test/test_apps_safety.c
	$(CC) $(CFLAGS) $(SRC_APP) test/test_apps_safety.c -o $(TARGET_APPS)

$(TARGET_STATE): $(SRC_STATE) test/test_state_machine.c
	$(CC) $(CFLAGS) $(SRC_STATE) test/test_state_machine.c -o $(TARGET_STATE)

$(TARGET_DERATE): $(SRC_DERATE) test/test_derate.c
	$(CC) $(CFLAGS) $(SRC_DERATE) test/test_derate.c -o $(TARGET_DERATE)

$(TARGET_CAN): $(SRC_CAN) test/test_can.c
	$(CC) $(CFLAGS) $(SRC_CAN) test/test_can.c -o $(TARGET_CAN)

clean:
	rm -f $(TARGET_APPS) $(TARGET_STATE) $(TARGET_DERATE) $(TARGET_CAN)

.PHONY: all clean