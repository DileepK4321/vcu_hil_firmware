#ifndef VCU_HAL_H
#define VCU_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "vcu_types.h"
#include "vcu_can.h"

/* Hardware Initialization */
void vcu_hal_init(void);

/* Peripheral Input Sampling (Called every 10ms control tick) */
void vcu_hal_read_inputs(vcu_inputs_t *inputs);

/* Peripheral Output Actuation (Called every 10ms control tick) */
void vcu_hal_write_outputs(const vcu_outputs_t *outputs);

/* CAN Hardware Hooks */
bool vcu_hal_can_transmit(const can_frame_t *frame);

#endif /* VCU_HAL_H */