#include "app_task.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void task_blink(void *args) {
    (void)args;
    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500)); // El LED siempre parpadeará vivo
    }
}

void task_control(void *args) {
    (void)args;
    
    uint16_t raw_pot1 = 0;
    uint16_t raw_pot2 = 0;

    while (1) {
        // BLOQUEO: Esperamos aquí indefinidamente hasta que el DMA termine
        // Esto libera la CPU para el Blink.
        // portMAX_DELAY significa "esperar para siempre"
        if (xSemaphoreTake(sem_adc_ready, portMAX_DELAY) == pdTRUE) {
            
            // --- SECCIÓN CRÍTICA (Lectura de buffer) ---
            // Como el DMA es circular, podría escribir mientras leemos.
            // En este caso simple de 16 bits es atómico, pero copiamos rápido.
            raw_pot1 = adc_dma_buffer[0];
            raw_pot2 = adc_dma_buffer[1];

            // Actualizamos PWM
            // Mapeo 4095 (ADC) -> 1000 (TIM)
            timer_set_oc_value(PWM_TIM, PWM1_CH, (raw_pot1 * 1000) / 4096);
            timer_set_oc_value(PWM_TIM, PWM2_CH, (raw_pot2 * 1000) / 4096);
            
            // Opcional: Si el ADC va muy rápido (microsegundos), podríamos saturar.
            // Como configuramos el ADC lento, esto debería ir a unos pocos kHz.
            // Si quisieras limitar la velocidad de actualización, podrías poner un delay aquí.
            // vTaskDelay(pdMS_TO_TICKS(10)); 
        }
    }
}