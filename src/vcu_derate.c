#include "vcu_derate.h"

static float clampf(float val, float min, float max)
{
    if(val<min) return min;
    if(val>max) return max;
    return val;
}

static float calculate_bms_temp_derate(float temp_c)
{
    if(temp_c <= BMS_TEMP_NOMINAL_C)
    {
        return 1.0f;
    }
    if(temp_c >= BMS_TEMP_CRITICAL_C)
    {
        return 0.0f;
    }
    float factor = 1.0 - ((temp_c-BMS_TEMP_NOMINAL_C)/(BMS_TEMP_CRITICAL_C-BMS_TEMP_NOMINAL_C));
    return clampf(factor,0.0f,1.0f); 
}

static float calculate_bms_volatage_derate(float volt_v)
{
    if(volt_v >= BMS_VOLT_NOMINAL_V)
    {
        return 1.0f;
    }
    if(volt_v <= BMS_VOLT_CRITICAL_V)
    {
        return 0.0f;
    }
    float factor = ((volt_v-BMS_VOLT_CRITICAL_V)/(BMS_VOLT_NOMINAL_V-BMS_VOLT_CRITICAL_V));
    return clampf(factor,0.0f,1.0f); 
}

static float calculate_inv_temp_derate(float temp_c)
{
    if(temp_c <= INV_TEMP_NOMINAL_C)
    {
        return 1.0f;
    }
    if(temp_c >= INV_TEMP_CRITICAL_C)
    {
        return 0.0f;
    }
    float factor = 1.0 - ((temp_c-INV_TEMP_NOMINAL_C)/(INV_TEMP_CRITICAL_C-INV_TEMP_NOMINAL_C));
    return clampf(factor,0.0f,1.0f); 
}

float vcu_derate_calculate_factor(const vcu_inputs_t *inputs)
{
    if (!inputs) return 0.0f;
    float k_bus_temp = calculate_bms_temp_derate(inputs->bms_max_cell_temp_c);
    float k_bus_voltage = calculate_bms_volatage_derate(inputs->bms_pack_voltage_v);
    float k_inv_temp = calculate_inv_temp_derate(inputs->inverter_temp_c);

    float k_derate = k_bus_temp;
    if(k_bus_voltage < k_derate)
    {
        k_derate = k_bus_voltage;
    }
    if(k_inv_temp<k_derate)
    {
        k_derate = k_inv_temp;
    }
    return k_derate;
}

float vcu_derate_apply(float raw_torque_nm, const vcu_inputs_t *inputs)
{
    float factor = vcu_derate_calculate_factor(inputs);
    return raw_torque_nm * factor;
}