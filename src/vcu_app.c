#include "vcu_app.h"
#include <math.h>

void vcu_app_init(vcu_inputs_t *inputs, vcu_outputs_t *outputs) {
    if (!inputs || !outputs) return;

    inputs->apps1_raw_adc = APPS_ADC_MIN_VALID;
    inputs->apps2_raw_adc = APPS_ADC_MIN_VALID;
    inputs->brake_switch_active = false;
    inputs->start_button_pressed = false;

    outputs->target_torque_nm = 0.0f;
    outputs->inverter_enable = false;
    outputs->main_contactor_pos_rel = false;
    outputs->main_contactor_neg_rel = false;
    outputs->precharge_relay = false;
    outputs->current_state = VCU_STATE_INIT;
    outputs->active_fault_mask = FAULT_NONE;
}

float vcu_app_process_apps(vcu_inputs_t *inputs, uint16_t *fault_mask) {
    if (!inputs || !fault_mask) return 0.0f;

    /* 1. Out-of-Range Boundary Check (Short to GND or VCC) */
    bool apps1_out_of_bounds = (inputs->apps1_raw_adc < APPS_ADC_MIN_VALID) || 
                               (inputs->apps1_raw_adc > APPS_ADC_MAX_VALID);
    bool apps2_out_of_bounds = (inputs->apps2_raw_adc < APPS_ADC_MIN_VALID) || 
                               (inputs->apps2_raw_adc > APPS_ADC_MAX_VALID);

    if (apps1_out_of_bounds || apps2_out_of_bounds) {
        *fault_mask |= FAULT_APPS_OUT_OF_RANGE;
        return 0.0f; /* Torque command forced to zero on fault */
    } else {
        *fault_mask &= ~FAULT_APPS_OUT_OF_RANGE;
    }

    /* 2. Normalize ADC counts to Percentage (0.0% to 100.0%) */
    float apps1_pct = ((float)(inputs->apps1_raw_adc - APPS_ADC_MIN_VALID) / 
                      (float)(APPS_ADC_MAX_VALID - APPS_ADC_MIN_VALID)) * 100.0f;
    float apps2_pct = ((float)(inputs->apps2_raw_adc - APPS_ADC_MIN_VALID) / 
                      (float)(APPS_ADC_MAX_VALID - APPS_ADC_MIN_VALID)) * 100.0f;

    /* 3. Plausibility Check (|APPS1 - APPS2| <= 10%) */
    float pedal_diff = fabsf(apps1_pct - apps2_pct);
    if (pedal_diff > APPS_PLAUSIBILITY_MAX_DIFF_PCT) {
        *fault_mask |= FAULT_APPS_MISMATCH;
        return 0.0f; /* Torque command forced to zero on fault */
    } else {
        *fault_mask &= ~FAULT_APPS_MISMATCH;
    }

    /* Average the two valid sensor readings */
    float resolved_pedal_pct = (apps1_pct + apps2_pct) / 2.0f;

    /* 4. Brake Override Rule (EV Safety Requirement) */
    static bool brake_override_active = false;
    if (inputs->brake_switch_active && (resolved_pedal_pct > 25.0f)) {
        brake_override_active = true;
    } else if (resolved_pedal_pct < 5.0f) {
        brake_override_active = false; /* Clear override when pedal released */
    }

    if (brake_override_active) {
        return 0.0f;
    }

    return resolved_pedal_pct;
}