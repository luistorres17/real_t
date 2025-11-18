#include "app_task.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"

void task_blink(void *args) {
    (void)args;
    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_control(void *args) {
    (void)args;
    uint16_t raw1, raw2;
    uint32_t duty1, duty2;

    // 1 seg al 50% para ver arranque
    timer_set_oc_value(PWM_TIM, PWM1_CH, 500);
    timer_set_oc_value(PWM_TIM, PWM2_CH, 500);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        // Leer
        raw1 = adc_read_blocking(1); // Pot 1
        raw2 = adc_read_blocking(2); // Pot 2

        // Verificación de Error
        if (raw1 == 0xFFFF || raw2 == 0xFFFF) {
            // ERROR: Timeout del ADC.
            // Ponemos PWM al 100% para gritar "ERROR DE HARDWARE"
            timer_set_oc_value(PWM_TIM, PWM1_CH, 999);
            timer_set_oc_value(PWM_TIM, PWM2_CH, 999);
        } else {
            // FUNCIONAMIENTO NORMAL
            duty1 = ((uint32_t)raw1 * 1000) / 4096;
            duty2 = ((uint32_t)raw2 * 1000) / 4096;

            timer_set_oc_value(PWM_TIM, PWM1_CH, duty1);
            timer_set_oc_value(PWM_TIM, PWM2_CH, duty2);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}