#include "vcu_can.h"

static uint32_t bms_timeout_counter = 0;
static uint32_t inv_timeout_counter = 0;

void vcu_can_init(void) {
    bms_timeout_counter = 0;
    inv_timeout_counter = 0;
}

bool vcu_can_unpack_bms(const can_frame_t *frame, vcu_inputs_t *inputs) {
    if (!frame || !inputs || frame->id != CAN_ID_BMS_STATUS || frame->dlc < 6) {
        return false;
    }

    /* Byte 0-1: Pack Voltage (16-bit uint, Little-Endian) */
    uint16_t volt_raw = (uint16_t)frame->data[0] | ((uint16_t)frame->data[1] << 8);
    inputs->bms_pack_voltage_v = (float)volt_raw * 0.1f;

    /* Byte 2-3: Pack Current (16-bit uint with -1000.0A offset) */
    uint16_t curr_raw = (uint16_t)frame->data[2] | ((uint16_t)frame->data[3] << 8);
    inputs->bms_pack_current_a = ((float)curr_raw * 0.1f) - 1000.0f;

    /* Byte 4: Max Cell Temperature (8-bit uint with -40C offset) */
    uint8_t temp_raw = frame->data[4];
    inputs->bms_max_cell_temp_c = (float)temp_raw - 40.0f;

    /* Byte 5: State of Charge (8-bit uint, 0.5% scale) */
    uint8_t soc_raw = frame->data[5];
    inputs->bms_soc_pct = (float)soc_raw * 0.5f;

    /* Reset supervision counter and mark heartbeat */
    bms_timeout_counter = 0;
    inputs->bms_heartbeat_rx = true;

    return true;
}

bool vcu_can_unpack_inverter(const can_frame_t *frame, vcu_inputs_t *inputs) {
    if (!frame || !inputs || frame->id != CAN_ID_INV_TELEMETRY || frame->dlc < 5) {
        return false;
    }

    /* Byte 0-1: Inverter DC Bus Voltage (16-bit uint, 0.1V scale) */
    uint16_t dc_bus_raw = (uint16_t)frame->data[0] | ((uint16_t)frame->data[1] << 8);
    inputs->inverter_dc_bus_v = (float)dc_bus_raw * 0.1f;

    /* Byte 2-3: Motor Speed (16-bit signed int, 1 RPM scale) */
    int16_t rpm_raw = (int16_t)((uint16_t)frame->data[2] | ((uint16_t)frame->data[3] << 8));
    inputs->motor_rpm = rpm_raw;

    /* Byte 4: Power Stage Temperature (8-bit uint with -40C offset) */
    uint8_t temp_raw = frame->data[4];
    inputs->inverter_temp_c = (float)temp_raw - 40.0f;

    /* Reset supervision counter and mark heartbeat */
    inv_timeout_counter = 0;
    inputs->inverter_heartbeat_rx = true;

    return true;
}

void vcu_can_check_timeouts(vcu_inputs_t *inputs, vcu_outputs_t *outputs) {
    if (!inputs || !outputs) return;

    bms_timeout_counter++;
    inv_timeout_counter++;

    /* Trip specific BMS timeout fault if message lost for > 100ms */
    if (bms_timeout_counter >= CAN_TIMEOUT_TICKS) {
        inputs->bms_heartbeat_rx = false;
        outputs->active_fault_mask |= FAULT_BMS_CAN_TIMEOUT;
    }

    /* Trip specific Inverter timeout fault if message lost for > 100ms */
    if (inv_timeout_counter >= CAN_TIMEOUT_TICKS) {
        inputs->inverter_heartbeat_rx = false;
        outputs->active_fault_mask |= FAULT_INVERTER_CAN_TIMEOUT;
    }
}

bool vcu_can_pack_cmd(const vcu_outputs_t *outputs, can_frame_t *frame) {
    if (!outputs || !frame) {
        return false;
    }

    frame->id = CAN_ID_VCU_CMD;
    frame->dlc = 8;

    /* Clear entire payload buffer first */
    for (int i = 0; i < 8; i++) {
        frame->data[i] = 0x00;
    }

    /* 1. Scale Target Torque (0.1 Nm/bit scale factor) */
    float clamped_torque = outputs->target_torque_nm;
    if (clamped_torque < 0.0f) clamped_torque = 0.0f;
    if (clamped_torque > 6553.5f) clamped_torque = 6553.5f;

    uint16_t torque_raw = (uint16_t)(clamped_torque * 10.0f + 0.5f); /* Round to nearest int */

    /* Byte 0-1: Pack Target Torque (Little-Endian) */
    frame->data[0] = (uint8_t)(torque_raw & 0xFFU);
    frame->data[1] = (uint8_t)((torque_raw >> 8) & 0xFFU);

    /* 2. Pack Control and Relay Status Flags into Byte 2 */
    uint8_t flags = 0;
    if (outputs->inverter_enable)         flags |= (1U << 0);
    if (outputs->main_contactor_pos_rel)  flags |= (1U << 1);
    if (outputs->main_contactor_neg_rel)  flags |= (1U << 2);
    if (outputs->precharge_relay)         flags |= (1U << 3);

    frame->data[2] = flags;

    return true;
}