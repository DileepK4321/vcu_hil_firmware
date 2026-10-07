#ifndef VCU_DERATE_H
#define VCU_DERATE_H

#include "vcu_types.h"

/*Derating limit thresholds*/
#define BMS_TEMP_NOMINAL_C  45.0f
#define BMS_TEMP_CRITICAL_C  60.0f

#define BMS_VOLT_NOMINAL_V  320.0f
#define BMS_VOLT_CRITICAL_V  300.0f

#define INV_TEMP_NOMINAL_C   70.0f
#define INV_TEMP_CRITICAL_C  85.0f

float vcu_derate_calculate_factor(const vcu_inputs_t *inputs);
float vcu_derate_apply(float raw_torque_nm, const vcu_inputs_t *inputs);

#endif  //VCU_DERATE_H
