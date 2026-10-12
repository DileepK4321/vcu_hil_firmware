#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "vcu_types.h"
#include "vcu_can.h"

static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            tests_passed++; \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s (Line %d): %s\n", __FILE__, __LINE__, msg); \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(actual, expected, tol, msg) \
    TEST_ASSERT(fabsf((actual) - (expected)) <= (tol), msg)

static void test_can_unpack_bms(void) {
    can_frame_t frame = {0};
    vcu_inputs_t inputs = {0};

    frame.id = CAN_ID_BMS_STATUS;
    frame.dlc = 6;
    frame.data[0] = 0xF4;
    frame.data[1] = 0x01;
    frame.data[2] = 0x38;
    frame.data[3] = 0x27;
    frame.data[4] = 0x50;
    frame.data[5] = 0x8C;

    bool success = vcu_can_unpack_bms(&frame, &inputs);
    TEST_ASSERT(success, "BMS CAN frame unpacking must return true for valid ID and DLC");
    TEST_ASSERT_FLOAT_EQ(inputs.bms_pack_voltage_v, 50.0f, 0.001f, "BMS pack voltage decoded incorrectly");
    TEST_ASSERT_FLOAT_EQ(inputs.bms_pack_current_a, 4.0f, 0.001f, "BMS pack current decoded incorrectly");
    TEST_ASSERT_FLOAT_EQ(inputs.bms_max_cell_temp_c, 40.0f, 0.001f, "BMS max cell temp decoded incorrectly");
    TEST_ASSERT_FLOAT_EQ(inputs.bms_soc_pct, 70.0f, 0.001f, "BMS SoC decoded incorrectly");
    TEST_ASSERT(inputs.bms_heartbeat_rx, "BMS heartbeat flag must be set true upon successful unpack");
}

static void test_can_unpack_inverter(void) {
    can_frame_t frame = {0};
    vcu_inputs_t inputs = {0};

    frame.id = CAN_ID_INV_TELEMETRY;
    frame.dlc = 5;
    frame.data[0] = 0x88;
    frame.data[1] = 0x0B;
    frame.data[2] = 0xB8;
    frame.data[3] = 0x0B;
    frame.data[4] = 0x64;

    bool success = vcu_can_unpack_inverter(&frame, &inputs);
    TEST_ASSERT(success, "Inverter CAN frame unpacking must return true");
    TEST_ASSERT_FLOAT_EQ(inputs.inverter_dc_bus_v, 295.2f, 0.001f, "Inverter DC bus decoded incorrectly");
    TEST_ASSERT(inputs.motor_rpm == 3000, "Motor RPM decoded incorrectly");
    TEST_ASSERT_FLOAT_EQ(inputs.inverter_temp_c, 60.0f, 0.001f, "Inverter temp decoded incorrectly");
    TEST_ASSERT(inputs.inverter_heartbeat_rx, "Inverter heartbeat flag must be set true");
}

static void test_can_bms_timeout(void) {
    vcu_can_init(); /* Reset static counters */
    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};
    outputs.active_fault_mask = FAULT_NONE;

    /* Simulate 9 ticks without BMS message (under threshold) */
    for (int i = 0; i < 9; i++) {
        vcu_can_check_timeouts(&inputs, &outputs);
    }
    TEST_ASSERT((outputs.active_fault_mask & FAULT_BMS_CAN_TIMEOUT) == 0, "BMS timeout must not trip before 10 ticks");

    /* 10th tick exceeds threshold */
    vcu_can_check_timeouts(&inputs, &outputs);
    TEST_ASSERT((outputs.active_fault_mask & FAULT_BMS_CAN_TIMEOUT) != 0, "FAULT_BMS_CAN_TIMEOUT must trip after 10 ticks");
}

static void test_can_inverter_timeout(void) {
    vcu_can_init(); /* Reset static counters */
    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};
    outputs.active_fault_mask = FAULT_NONE;

    /* Simulate 9 ticks without Inverter message (under threshold) */
    for (int i = 0; i < 9; i++) {
        vcu_can_check_timeouts(&inputs, &outputs);
    }
    TEST_ASSERT((outputs.active_fault_mask & FAULT_INVERTER_CAN_TIMEOUT) == 0, "Inverter timeout must not trip before 10 ticks");

    /* 10th tick exceeds threshold */
    vcu_can_check_timeouts(&inputs, &outputs);
    TEST_ASSERT((outputs.active_fault_mask & FAULT_INVERTER_CAN_TIMEOUT) != 0, "FAULT_INVERTER_CAN_TIMEOUT must trip after 10 ticks");
}

static void test_can_pack_cmd(void) {
    vcu_outputs_t outputs = {0};
    can_frame_t frame = {0};

    outputs.target_torque_nm = 150.5f;       /* 150.5 * 10 = 1505 (0x05E1) */
    outputs.inverter_enable = true;          /* Bit 0 */
    outputs.main_contactor_pos_rel = true;   /* Bit 1 */
    outputs.main_contactor_neg_rel = false;  /* Bit 2 */
    outputs.precharge_relay = true;          /* Bit 3 */

    bool success = vcu_can_pack_cmd(&outputs, &frame);
    TEST_ASSERT(success, "Command packing must return true");
    TEST_ASSERT(frame.id == CAN_ID_VCU_CMD, "Outbound CAN ID must match 0x300");
    TEST_ASSERT(frame.dlc == 8, "DLC must be 8 bytes");
    
    /* Verify torque raw bytes (Little-Endian 0x05E1) */
    TEST_ASSERT(frame.data[0] == 0xE1, "Torque low byte packed incorrectly");
    TEST_ASSERT(frame.data[1] == 0x05, "Torque high byte packed incorrectly");

    /* Verify control flags byte (Bits 0, 1, 3 set -> 0x0B) */
    TEST_ASSERT(frame.data[2] == 0x0B, "Control relay flags packed incorrectly");
}

int main(void) {
    printf("=========================================\n");
    printf("     RUNNING VCU CAN MIDDLEWARE TESTS    \n");
    printf("=========================================\n");

    test_can_unpack_bms();
    test_can_unpack_inverter();
    test_can_bms_timeout();
    test_can_inverter_timeout();
    test_can_pack_cmd();

    printf("-----------------------------------------\n");
    printf("Results: %u PASSED, %u FAILED\n", tests_passed, tests_failed);
    printf("=========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}