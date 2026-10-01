#ifndef VCU_TYPES_H
#define VCU_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Top-Level VCU State Machine States */
typedef enum {
    VCU_STATE_INIT = 0,         /* Hardware check & peripheral startup */
    VCU_STATE_STANDBY,          /* Ignition ON, high-voltage contactors open */
    VCU_STATE_PRECHARGE,        /* Closing precharge relay, waiting for DC-bus voltage sync */
    VCU_STATE_READY_TO_DRIVE,   /* Contactors closed, inverter active, torque enabled */
    VCU_STATE_CHARGE,           /* EVSE connected, charging handshake active */
    VCU_STATE_LIMP_MODE,        /* Derated power mode due to thermal/voltage limits */
    VCU_STATE_FAULT_SHUTDOWN    /* Critical fault: contactors open, zero torque command */
} vcu_state_t;

/* System Fault Flags (Bitfield Representation) */
typedef enum {
    FAULT_NONE                  = 0x0000,
    FAULT_APPS_MISMATCH         = (1 << 0), /* Accelerator pedal sensors out of sync (>10%) */
    FAULT_APPS_OUT_OF_RANGE     = (1 << 1), /* Pedal voltage out of valid bounds (0.5V - 4.5V) */
    FAULT_BMS_CAN_TIMEOUT       = (1 << 2), /* No CAN message from BMS for >500ms */
    FAULT_INVERTER_CAN_TIMEOUT  = (1 << 3), /* No CAN message from Inverter for >500ms */
    FAULT_BMS_OVERTEMP          = (1 << 4), /* Pack temp exceeds max limit */
    FAULT_BMS_UNDERVOLT         = (1 << 5), /* Pack voltage dropped below threshold */
    FAULT_PRECHARGE_FAILURE     = (1 << 6)  /* DC-bus failed to reach 90% pack voltage in time */
} vcu_fault_flag_t;

/* Physical VCU Inputs (ADC + CAN Telemetry) */
typedef struct {
    /* Driver Interface (Analog ADC Inputs) */
    uint16_t apps1_raw_adc;      /* Pedal Sensor 1 (0 to 4095 for 12-bit ADC) */
    uint16_t apps2_raw_adc;      /* Pedal Sensor 2 (0 to 4095 for 12-bit ADC) */
    bool brake_switch_active;    /* Digital brake pedal switch input */
    bool start_button_pressed;   /* Start/Stop switch input */

    /* BMS Telemetry (Received via CAN) */
    float bms_pack_voltage_v;    /* Live pack voltage */
    float bms_pack_current_a;    /* Live pack current (+ discharge, - charge) */
    float bms_max_cell_temp_c;   /* Highest cell temperature in pack */
    float bms_soc_pct;           /* State of Charge (0.0% to 100.0%) */
    bool bms_heartbeat_rx;       /* Heartbeat flag toggled by CAN RX ISR */

    /* Inverter Telemetry (Received via CAN) */
    float inverter_dc_bus_v;     /* Inverter internal DC-bus voltage */
    int16_t motor_rpm;           /* Live motor speed */
    bool inverter_heartbeat_rx;  /* Heartbeat flag toggled by CAN RX ISR */
} vcu_inputs_t;

/* Physical VCU Outputs (Torque Command + Relay Signals) */
typedef struct {
    /* Motor Command (Transmitted via CAN) */
    float target_torque_nm;      /* Torque command sent to Inverter (-200Nm to +200Nm) */
    bool inverter_enable;        /* High-level inverter enable switch */

    /* High-Voltage Contactor Relays (Digital Outputs) */
    bool main_contactor_pos_rel; /* Positive main contactor relay command */
    bool main_contactor_neg_rel; /* Negative main contactor relay command */
    bool precharge_relay;        /* Precharge circuit relay command */

    /* Status Indicators */
    vcu_state_t current_state;   /* Active vehicle state */
    uint16_t active_fault_mask;  /* Bitwise active fault mask */
} vcu_outputs_t;

#endif /* VCU_TYPES_H */