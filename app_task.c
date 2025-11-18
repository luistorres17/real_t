#include "app_task.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void task_blink(void *args) {
    (void)args;
    while (1) {
        // Parpadeo visual para confirmar que el RTOS corre
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_control(void *args) {
    (void)args;
    uint32_t duty = 0;

    while (1) {
        // 1. Pedir lectura
        adc_read_single();

        // 2. Esperar resultado (Timeout 20ms)
        if (xSemaphoreTake(sem_adc_ready, pdMS_TO_TICKS(20)) == pdTRUE) {
            
            // Tenemos el dato en 'adc_val' (0 - 4095)
            // Mapeo simple a (0 - 1000)
            duty = ((uint32_t)adc_val * 1000) / 4096;

            timer_set_oc_value(PWM_TIM, PWM_CH, duty);
        } else {
            // Si llegamos aquí, el ADC/DMA falló (Timeout).
            // Podríamos prender un LED de error o reiniciar el ADC.
            // Por ahora, no hacemos nada, solo reintentamos en el siguiente ciclo.
        }

        // Refresco de 50ms (20Hz)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}