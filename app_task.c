#include "app_task.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pid_controller.h"

void task_blink(void *args) {
    (void)args;
    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_control(void *args) {
    (void)args;
    
    uint32_t timeout_count = 0;
    
    // PID Controller - siempre activo para mantener 10 kHz
    PID_Controller freq_pid;
    PID_Init(&freq_pid, 0.05f, 0.01f, 0.001f, 0.05f);
    
    float measured_freq = 10000.0f;
    float error = 0.0f;
    float period_correction = 0.0f;
    int32_t new_period = 0;
    
    const uint32_t TIM2_CLOCK = 1000000; // 1 MHz
    const uint32_t BASE_PERIOD = 99;     // Periodo para 10 kHz
    
    // Validar que el semáforo fue creado correctamente
    if (sem_adc_ready == NULL) {
        vTaskSuspend(NULL);
    }

    while (1) {
        adc_start_scan();

        if (xSemaphoreTake(sem_adc_ready, pdMS_TO_TICKS(100)) == pdTRUE) {
            timeout_count = 0;
            
            // Medir frecuencia actual del PWM
            uint32_t current_period = TIM_ARR(PWM1_TIM);
            measured_freq = (float)TIM2_CLOCK / (float)(current_period + 1);
            
            // Calcular error: queremos mantener 10 kHz
            error = freq_pid.setpoint - measured_freq;
            
            // PID calcula corrección de periodo
            // Relación inversa: si necesitamos más frecuencia, reducimos periodo
            period_correction = PID_Calculate(&freq_pid, error);
            
            // Aplicar corrección al periodo base
            new_period = (int32_t)BASE_PERIOD - (int32_t)period_correction;
            
            // Limitar periodo para evitar saturación
            if (new_period < 49) new_period = 49;    // Max 20 kHz
            if (new_period > 200) new_period = 200;  // Min 5 kHz
            
            // Aplicar al timer
            timer_set_period(PWM1_TIM, (uint32_t)new_period);
            timer_set_oc_value(PWM1_TIM, PWM1_CH, (uint32_t)new_period / 2);
            
        } else {
            timeout_count++;
            if (timeout_count > 10) {
                adc_setup();
                timeout_count = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}