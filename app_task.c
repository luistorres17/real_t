#include "app_task.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void task_blink(void *args) {
    (void)args;
    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500)); // LED parpadea a ritmo normal
    }
}

void task_control(void *args) {
    (void)args;
    uint32_t duty1 = 0;
    uint32_t duty2 = 0;

    // Prueba inicial 1 seg
    timer_set_oc_value(PWM_TIM, PWM1_CH, 500);
    timer_set_oc_value(PWM_TIM, PWM2_CH, 500);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        // 1. Pedir al hardware que trabaje
        adc_start_scan();

        // 2. Dormir hasta que el DMA termine (Eficiencia máxima)
        if (xSemaphoreTake(sem_adc_ready, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            // ¡DATOS LLEGARON POR DMA!
            duty1 = ((uint32_t)adc_dma_buffer[0] * 1000) / 4096;
            duty2 = ((uint32_t)adc_dma_buffer[1] * 1000) / 4096;

            timer_set_oc_value(PWM_TIM, PWM1_CH, duty1);
            timer_set_oc_value(PWM_TIM, PWM2_CH, duty2);
        } else {
            // Timeout (No debería pasar ahora que arreglamos el trigger)
            // Indicador de error visual
            timer_set_oc_value(PWM_TIM, PWM1_CH, 100);
            timer_set_oc_value(PWM_TIM, PWM2_CH, 100);
        }

        // 3. Tasa de refresco 20Hz
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}