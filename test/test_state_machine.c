#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "vcu_types.h"
#include "vcu_app.h"
#include "vcu_state.h"

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

static void test_precharge_success(void) {
    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};

    vcu_state_init(&outputs);
    TEST_ASSERT(outputs.current_state == VCU_STATE_INIT, "Initial state must be INIT");

    /* Step 1: INIT -> STANDBY */
    vcu_state_step(&inputs, &outputs);
    TEST_ASSERT(outputs.current_state == VCU_STATE_STANDBY, "State should advance to STANDBY");

    /* Step 2: Press Start + Brake -> PRECHARGE */
    inputs.start_button_pressed = true;
    inputs.brake_switch_active = true;
    vcu_state_step(&inputs, &outputs);
    TEST_ASSERT(outputs.current_state == VCU_STATE_PRECHARGE, "State should advance to PRECHARGE");
    TEST_ASSERT(outputs.precharge_relay == true, "Precharge relay must be closed");

    /* Step 3: DC Bus reaches 90% of pack voltage -> READY_TO_DRIVE */
    inputs.bms_pack_voltage_v = 400.0f;
    inputs.inverter_dc_bus_v = 365.0f; /* > 360V (90% of 400V) */
    vcu_state_step(&inputs, &outputs);

    TEST_ASSERT(outputs.current_state == VCU_STATE_READY_TO_DRIVE, "State should advance to READY_TO_DRIVE");
    TEST_ASSERT(outputs.main_contactor_pos_rel == true, "Main positive contactor must be closed");
    TEST_ASSERT(outputs.main_contactor_neg_rel == true, "Main negative contactor must be closed");
    TEST_ASSERT(outputs.precharge_relay == false, "Precharge relay must be opened");
    TEST_ASSERT(outputs.inverter_enable == true, "Inverter enable signal must be active");
}

static void test_precharge_timeout_fault(void) {
    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};

    vcu_state_init(&outputs);
    vcu_state_step(&inputs, &outputs); /* INIT -> STANDBY */

    inputs.start_button_pressed = true;
    inputs.brake_switch_active = true;
    vcu_state_step(&inputs, &outputs); /* STANDBY -> PRECHARGE */

    inputs.bms_pack_voltage_v = 400.0f;
    inputs.inverter_dc_bus_v = 0.0f; /* Failed precharge circuit */

    /* Run state machine up to 50 ticks (PRECHARGE_TIMEOUT_TICKS) */
    for (uint32_t i = 0; i < PRECHARGE_TIMEOUT_TICKS; i++) {
        vcu_state_step(&inputs, &outputs);
    }

    TEST_ASSERT(outputs.current_state == VCU_STATE_FAULT_SHUTDOWN, "Precharge timeout must force FAULT_SHUTDOWN");
    TEST_ASSERT((outputs.active_fault_mask & FAULT_PRECHARGE_FAILURE) != 0, "FAULT_PRECHARGE_FAILURE bit must be set");
    TEST_ASSERT(outputs.precharge_relay == false, "Precharge relay must be opened on fault");
}

static void test_ready_to_drive_torque(void) {
    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};

    /* Advance to READY_TO_DRIVE state */
    vcu_state_init(&outputs);
    vcu_state_step(&inputs, &outputs);
    inputs.start_button_pressed = true;
    inputs.brake_switch_active = true;
    vcu_state_step(&inputs, &outputs);
    inputs.bms_pack_voltage_v = 400.0f;
    inputs.inverter_dc_bus_v = 380.0f;
    vcu_state_step(&inputs, &outputs);

    /* Release start button & brake, set pedal to 50% (2050 ADC counts) */
    inputs.start_button_pressed = false;
    inputs.brake_switch_active = false;
    inputs.apps1_raw_adc = 2050;
    inputs.apps2_raw_adc = 2050;

    vcu_state_step(&inputs, &outputs);

    TEST_ASSERT(outputs.current_state == VCU_STATE_READY_TO_DRIVE, "System must remain in READY_TO_DRIVE");
    TEST_ASSERT(fabsf(outputs.target_torque_nm - 100.0f) < 1.0f, "50% pedal demand should output 100Nm target torque");
}

static void test_r2d_fault_trip(void) {
    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};

    /* Advance to READY_TO_DRIVE state */
    vcu_state_init(&outputs);
    vcu_state_step(&inputs, &outputs);
    inputs.start_button_pressed = true;
    inputs.brake_switch_active = true;
    vcu_state_step(&inputs, &outputs);
    inputs.bms_pack_voltage_v = 400.0f;
    inputs.inverter_dc_bus_v = 380.0f;
    vcu_state_step(&inputs, &outputs);

    /* Inject APPS sensor mismatch fault */
    inputs.start_button_pressed = false;
    inputs.brake_switch_active = false;
    inputs.apps1_raw_adc = 3040; /* 80% */
    inputs.apps2_raw_adc = 2380; /* 60% -> 20% drift */

    vcu_state_step(&inputs, &outputs);

    TEST_ASSERT(outputs.current_state == VCU_STATE_FAULT_SHUTDOWN, "APPS mismatch fault must cause state transition to FAULT_SHUTDOWN");
    TEST_ASSERT(outputs.target_torque_nm == 0.0f, "Torque must drop immediately to 0Nm on fault");
    TEST_ASSERT(outputs.inverter_enable == false, "Inverter must be disabled on fault");
    TEST_ASSERT(outputs.main_contactor_pos_rel == false, "Main positive contactor must open on fault");
}

int main(void) {
    printf("=========================================\n");
    printf("   RUNNING VCU SIL STATE MACHINE TESTS   \n");
    printf("=========================================\n");

    test_precharge_success();
    test_precharge_timeout_fault();
    test_ready_to_drive_torque();
    test_r2d_fault_trip();

    printf("-----------------------------------------\n");
    printf("Results: %u PASSED, %u FAILED\n", tests_passed, tests_failed);
    printf("=========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}