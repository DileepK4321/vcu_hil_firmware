#ifndef VCU_APP_H
#define VCU_APP_H

#include "vcu_types.h"

/* Calibration Constants for 12-bit ADC (3.3V ref) */
#define APPS_ADC_MIN_VALID      400U   /* ~0.48V: Below this indicates short to GND */
#define APPS_ADC_MAX_VALID      3700U  /* ~4.47V: Above this indicates short to VCC */
#define APPS_PLAUSIBILITY_MAX_DIFF_PCT 10.0f /* Max allowed drift between sensor 1 & 2 */

/* API Functions */
void vcu_app_init(vcu_inputs_t *inputs, vcu_outputs_t *outputs);
float vcu_app_process_apps(vcu_inputs_t *inputs, uint16_t *fault_mask);

#endif //VCU_APP_H