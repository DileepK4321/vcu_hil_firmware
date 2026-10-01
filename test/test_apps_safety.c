#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "vcu_app.h"

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

static void test_apps_normal_operation(void) {
    vcu_inputs_t inputs;
    vcu_outputs_t outputs;
    uint16_t fault_mask = FAULT_NONE;

    vcu_app_init(&inputs, &outputs);

    /* 2050 ADC counts = ~50% pedal position on both sensors */
    inputs.apps1_raw_adc = 2050;
    inputs.apps2_raw_adc = 2050;

    float pedal_pct = vcu_app_process_apps(&inputs, &fault_mask);

    TEST_ASSERT(fabsf(pedal_pct - 50.0f) < 0.5f, "Normal operation pedal % expected 50%");
    TEST_ASSERT(fault_mask == FAULT_NONE, "No faults should be set during normal operation");
}

static void test_apps_out_of_bounds(void) {
    vcu_inputs_t inputs;
    vcu_outputs_t outputs;
    uint16_t fault_mask = FAULT_NONE;

    vcu_app_init(&inputs, &outputs);

    /* Inject Short-to-GND on APPS1 (< 400 ADC counts) */
    inputs.apps1_raw_adc = 200; 
    inputs.apps2_raw_adc = 2050;

    float pedal_pct = vcu_app_process_apps(&inputs, &fault_mask);

    TEST_ASSERT(pedal_pct == 0.0f, "Out-of-bounds must force 0% pedal command");
    TEST_ASSERT((fault_mask & FAULT_APPS_OUT_OF_RANGE) != 0, "FAULT_APPS_OUT_OF_RANGE must be set");
}

static void test_apps_mismatch_fault(void) {
    vcu_inputs_t inputs;
    vcu_outputs_t outputs;
    uint16_t fault_mask = FAULT_NONE;

    vcu_app_init(&inputs, &outputs);

    /* APPS1 = 80% (3040 ADC), APPS2 = 60% (2380 ADC) -> 20% drift (>10% limit) */
    inputs.apps1_raw_adc = 3040;
    inputs.apps2_raw_adc = 2380;

    float pedal_pct = vcu_app_process_apps(&inputs, &fault_mask);

    TEST_ASSERT(pedal_pct == 0.0f, "APPS mismatch >10% must force 0% pedal command");
    TEST_ASSERT((fault_mask & FAULT_APPS_MISMATCH) != 0, "FAULT_APPS_MISMATCH must be set");
}

static void test_apps_brake_override(void) {
    vcu_inputs_t inputs;
    vcu_outputs_t outputs;
    uint16_t fault_mask = FAULT_NONE;

    vcu_app_init(&inputs, &outputs);

    /* Set APPS to 30% and press brake */
    inputs.apps1_raw_adc = 1390;
    inputs.apps2_raw_adc = 1390;
    inputs.brake_switch_active = true;

    float pedal_pct = vcu_app_process_apps(&inputs, &fault_mask);
    TEST_ASSERT(pedal_pct == 0.0f, "Brake active with APPS > 25% must force 0% output");

    /* Release brake but keep APPS at 20% -> Override must remain latched */
    inputs.brake_switch_active = false;
    pedal_pct = vcu_app_process_apps(&inputs, &fault_mask);
    TEST_ASSERT(pedal_pct == 0.0f, "Brake override must remain active until pedal < 5%");

    /* Drop APPS to 3% -> Override clears */
    inputs.apps1_raw_adc = 500;
    inputs.apps2_raw_adc = 500;
    pedal_pct = vcu_app_process_apps(&inputs, &fault_mask);
    TEST_ASSERT(pedal_pct < 5.0f, "Brake override should clear when pedal drops < 5%");
}

int main(void) {
    printf("=========================================\n");
    printf("   RUNNING VCU SIL APPS SAFETY UNIT TESTS \n");
    printf("=========================================\n");

    test_apps_normal_operation();
    test_apps_out_of_bounds();
    test_apps_mismatch_fault();
    test_apps_brake_override();

    printf("-----------------------------------------\n");
    printf("Results: %u PASSED, %u FAILED\n", tests_passed, tests_failed);
    printf("=========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}