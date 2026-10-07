#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "vcu_types.h"
#include "vcu_derate.h"

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

static void test_nominal_no_derating(void) {
    vcu_inputs_t inputs = {0};
    inputs.bms_max_cell_temp_c = 30.0f; /* <= 45C nominal */
    inputs.bms_pack_voltage_v  = 350.0f; /* >= 320V nominal */
    inputs.inverter_temp_c     = 50.0f;  /* <= 70C nominal */

    float k_factor = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_factor, 1.0f, 0.001f, "Nominal parameters must yield 1.0 derating factor");

    float torque_out = vcu_derate_apply(150.0f, &inputs);
    TEST_ASSERT_FLOAT_EQ(torque_out, 150.0f, 0.001f, "150Nm input torque must pass unattenuated under nominal conditions");
}

static void test_bms_thermal_derating_curve(void) {
    vcu_inputs_t inputs = {0};
    inputs.bms_pack_voltage_v = 350.0f;
    inputs.inverter_temp_c    = 50.0f;

    /* Mid-point test: 52.5C is halfway between 45C and 60C -> Factor 0.5 */
    inputs.bms_max_cell_temp_c = 52.5f;
    float k_mid = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_mid, 0.5f, 0.001f, "52.5C BMS cell temp must calculate 0.5 derating factor");

    float torque_mid = vcu_derate_apply(200.0f, &inputs);
    TEST_ASSERT_FLOAT_EQ(torque_mid, 100.0f, 0.001f, "200Nm torque must be scaled to 100Nm at 0.5 factor");

    /* Over-temp limit: 62.0C >= 60.0C threshold -> Factor 0.0 */
    inputs.bms_max_cell_temp_c = 62.0f;
    float k_over = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_over, 0.0f, 0.001f, "Over-temp >= 60C must enforce 0.0 derating factor");
}

static void test_voltage_sag_derating_curve(void) {
    vcu_inputs_t inputs = {0};
    inputs.bms_max_cell_temp_c = 30.0f;
    inputs.inverter_temp_c     = 50.0f;

    /* Mid-point test: 310V is halfway between 300V and 320V -> Factor 0.5 */
    inputs.bms_pack_voltage_v = 310.0f;
    float k_mid = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_mid, 0.5f, 0.001f, "310V sag must calculate 0.5 derating factor");

    /* Under-voltage limit: 290V <= 300V threshold -> Factor 0.0 */
    inputs.bms_pack_voltage_v = 290.0f;
    float k_under = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_under, 0.0f, 0.001f, "Voltage sag <= 300V must enforce 0.0 derating factor");
}

static void test_inverter_thermal_derating_curve(void) {
    vcu_inputs_t inputs = {0};
    inputs.bms_max_cell_temp_c = 30.0f;
    inputs.bms_pack_voltage_v  = 350.0f;

    /* Mid-point test: 77.5C is halfway between 70C and 85C -> Factor 0.5 */
    inputs.inverter_temp_c = 77.5f;
    float k_mid = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_mid, 0.5f, 0.001f, "77.5C inverter temp must calculate 0.5 derating factor");

    /* Over-temp limit: 88.0C >= 85.0C threshold -> Factor 0.0 */
    inputs.inverter_temp_c = 88.0f;
    float k_over = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_over, 0.0f, 0.001f, "Inverter temp >= 85C must enforce 0.0 derating factor");
}

static void test_worst_case_bottleneck(void) {
    vcu_inputs_t inputs = {0};
    
    /* Channel limits:
     * BMS Temp = 52.5C -> Factor 0.5
     * Pack Volt = 305V  -> Factor 0.25 (305V - 300V) / (320V - 300V)
     * Inv Temp  = 50.0C -> Factor 1.0
     * Bottleneck = min(0.5, 0.25, 1.0) = 0.25
     */
    inputs.bms_max_cell_temp_c = 52.5f;
    inputs.bms_pack_voltage_v  = 305.0f;
    inputs.inverter_temp_c     = 50.0f;

    float k_bottleneck = vcu_derate_calculate_factor(&inputs);
    TEST_ASSERT_FLOAT_EQ(k_bottleneck, 0.25f, 0.001f, "Engine must select lowest bottleneck factor (0.25)");

    float torque_out = vcu_derate_apply(200.0f, &inputs);
    TEST_ASSERT_FLOAT_EQ(torque_out, 50.0f, 0.001f, "200Nm torque scaled by 0.25 factor must equal 50Nm");
}

int main(void) {
    printf("=========================================\n");
    printf("   RUNNING VCU SIL DERATING ENGINE TESTS \n");
    printf("=========================================\n");

    test_nominal_no_derating();
    test_bms_thermal_derating_curve();
    test_voltage_sag_derating_curve();
    test_inverter_thermal_derating_curve();
    test_worst_case_bottleneck();

    printf("-----------------------------------------\n");
    printf("Results: %u PASSED, %u FAILED\n", tests_passed, tests_failed);
    printf("=========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}