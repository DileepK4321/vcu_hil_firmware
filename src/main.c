#include "vcu_types.h"
#include "vcu_app.h"
#include "vcu_state.h"
#include "vcu_derate.h"
#include "vcu_can.h"
#include "vcu_hal.h"

int main(void) {
    /* Initialize Hardware Peripherals */
    vcu_hal_init();

    vcu_inputs_t inputs = {0};
    vcu_outputs_t outputs = {0};

    outputs.current_state = VCU_STATE_INIT;
    outputs.active_fault_mask = FAULT_NONE;

    /* Main Superloop (10ms Periodic Tick) */
    while (1) {
        /* 1. Read physical inputs from HAL (ADC DMA, GPIO, CAN ISR flags) */
        vcu_hal_read_inputs(&inputs);

        /* 2. Check CAN communication timeouts */
        vcu_can_check_timeouts(&inputs, &outputs);

        /* 3. Execute State Machine Transition & Logic */
        vcu_state_step(&inputs, &outputs);

        /* 4. Actuate hardware outputs (Relays, CAN TX command) */
        vcu_hal_write_outputs(&outputs);

        /* 5. Periodic delay / RTOS tick sync for 10ms cycle */
        /* HAL_Delay(10); */
    }
}