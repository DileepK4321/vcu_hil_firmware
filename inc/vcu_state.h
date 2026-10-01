#ifndef VCU_STATE_H
#define VCU_STATE_H

#include "vcu_types.h"

/* Precharge Configuration Constants */
#define PRECHARGE_TIMEOUT_TICKS    50U    /* 50 * 10ms = 500ms timeout threshold */
#define PRECHARGE_VOLTAGE_RATIO    0.90f  /* DC-Bus must reach 90% of pack voltage */
#define MAX_TORQUE_RATED_NM        200.0f /* Maximum motor output torque */

/* API Functions */
void vcu_state_init(vcu_outputs_t *outputs);
void vcu_state_step(vcu_inputs_t *inputs, vcu_outputs_t *outputs);

#endif /* VCU_STATE_H */