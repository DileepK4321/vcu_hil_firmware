#include "vcu_state.h"
#include "vcu_app.h"
#include "vcu_derate.h"

static uint32_t precharge_timer_ticks = 0;

void vcu_state_init(vcu_outputs_t *outputs) {
    if (!outputs) return;

    precharge_timer_ticks = 0;
    outputs->current_state = VCU_STATE_INIT;
    outputs->active_fault_mask = FAULT_NONE;
    outputs->target_torque_nm = 0.0f;
    outputs->inverter_enable = false;
    outputs->main_contactor_pos_rel = false;
    outputs->main_contactor_neg_rel = false;
    outputs->precharge_relay = false;
}

void vcu_state_step(vcu_inputs_t *inputs, vcu_outputs_t *outputs) {
    if (!inputs || !outputs) return;

    /* ----------------------------------------------------------------- */
    /* STAGE 1: EVALUATE STATE TRANSITIONS                               */
    /* ----------------------------------------------------------------- */
    switch (outputs->current_state) {

        case VCU_STATE_INIT:
            precharge_timer_ticks = 0;
            outputs->current_state = VCU_STATE_STANDBY;
            break;

        case VCU_STATE_STANDBY:
            /* Transition condition: Start button pressed with brake applied */
            if (inputs->start_button_pressed && inputs->brake_switch_active) {
                precharge_timer_ticks = 0;
                outputs->current_state = VCU_STATE_PRECHARGE;
            }
            break;

        case VCU_STATE_PRECHARGE:
            precharge_timer_ticks++;

            float target_voltage = inputs->bms_pack_voltage_v * PRECHARGE_VOLTAGE_RATIO;
            if (inputs->inverter_dc_bus_v >= target_voltage && inputs->bms_pack_voltage_v > 0.0f) {
                /* Synchronized -> Advance to READY_TO_DRIVE */
                outputs->current_state = VCU_STATE_READY_TO_DRIVE;
            } 
            else if (precharge_timer_ticks >= PRECHARGE_TIMEOUT_TICKS) {
                /* Precharge Timeout -> Trip Fault */
                outputs->active_fault_mask |= FAULT_PRECHARGE_FAILURE;
                outputs->current_state = VCU_STATE_FAULT_SHUTDOWN;
            }
            break;

        case VCU_STATE_READY_TO_DRIVE: {
            /* Process APPS inputs and update fault mask */
            float pedal_pct = vcu_app_process_apps(inputs, &outputs->active_fault_mask);

            /* Any active fault forces immediate FAULT_SHUTDOWN state */
            if (outputs->active_fault_mask != FAULT_NONE) {
                outputs->current_state = VCU_STATE_FAULT_SHUTDOWN;
            } else {
                float raw_torque_nm = (pedal_pct / 100.0f) * MAX_TORQUE_RATED_NM;
                outputs->target_torque_nm = vcu_derate_apply(raw_torque_nm, inputs);
            }
            break;
        }

        case VCU_STATE_FAULT_SHUTDOWN:
        default:
            /* Safety interlock remains latched */
            break;
    }

    /* ----------------------------------------------------------------- */
    /* STAGE 2: UPDATE OUTPUTS IMMEDIATELY BASED ON RESOLVED STATE       */
    /* ----------------------------------------------------------------- */
    switch (outputs->current_state) {

        case VCU_STATE_INIT:
        case VCU_STATE_STANDBY:
            outputs->target_torque_nm = 0.0f;
            outputs->inverter_enable = false;
            outputs->main_contactor_pos_rel = false;
            outputs->main_contactor_neg_rel = false;
            outputs->precharge_relay = false;
            break;

        case VCU_STATE_PRECHARGE:
            outputs->target_torque_nm = 0.0f;
            outputs->inverter_enable = false;
            outputs->main_contactor_pos_rel = false;
            outputs->main_contactor_neg_rel = false;
            outputs->precharge_relay = true;
            break;

        case VCU_STATE_READY_TO_DRIVE:
            outputs->inverter_enable = true;
            outputs->main_contactor_pos_rel = true;
            outputs->main_contactor_neg_rel = true;
            outputs->precharge_relay = false;
            break;

        case VCU_STATE_FAULT_SHUTDOWN:
        default:
            outputs->target_torque_nm = 0.0f;
            outputs->inverter_enable = false;
            outputs->main_contactor_pos_rel = false;
            outputs->main_contactor_neg_rel = false;
            outputs->precharge_relay = false;
            break;
    }
}