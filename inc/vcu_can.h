#ifndef VCU_CAN_H
#define VCU_CAN_H

#include<stdio.h>
#include<stdbool.h>
#include "vcu_types.h"

/* can frames*/
#define CAN_ID_BMS_STATUS    0x100U
#define CAN_ID_INV_TELEMETRY 0x200U
#define CAN_ID_VCU_CMD       0x300U

#define CAN_TIMEOUT_TICKS   10U

typedef struct 
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
}can_frame_t;

void vcu_can_init(void);
bool vcu_can_unpack_bms(const can_frame_t *frame, vcu_inputs_t *inputs);
bool vcu_can_unpack_inverter(const can_frame_t *frame, vcu_inputs_t *inputs);
bool vcu_can_pack_cmd(const vcu_outputs_t *outputs, can_frame_t *frame);
void vcu_can_check_timeouts(vcu_inputs_t *inputs, vcu_outputs_t *outputs);

#endif