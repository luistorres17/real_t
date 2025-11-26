#include "app_task.h"
#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pid_controller.h"

static float map_range(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void task_blink(void *args) {
    (void)args;
    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// --- Tarea del Botón de Pánico / Turbo ---
void task_turbo(void *args) {
    (void)args;
    const uint32_t TIM2_CLOCK = 1000000;

    while (1) {
        // Esperar a que se presione el botón (Bloqueado hasta que la ISR lo despierte)
        if (xSemaphoreTake(sem_button_pressed, portMAX_DELAY) == pdTRUE) {
            
            // 1. Tomar el control EXCLUSIVO del PWM
            // Al tener el Mutex, la tarea del PID (task_control) se bloqueará si intenta usarlo.
            if (xSemaphoreTake(mutex_pwm_resource, pdMS_TO_TICKS(100)) == pdTRUE) {
                
                // Leer configuración actual
                uint32_t current_arr = TIM_ARR(PWM1_TIM);
                
                // Calcular periodo para el DOBLE de frecuencia
                // Freq = 1MHz / (ARR+1)
                // Doble Freq = 1MHz / ((ARR+1)/2) -> Nuevo ARR = ((ARR+1)/2) - 1
                uint32_t new_arr = ((current_arr + 1) / 2) - 1;
                
                // Limite de seguridad (no bajar de 25kHz aprox)
                if (new_arr < 40) new_arr = 40;

                // Aplicar Hardware Inmediatamente
                timer_set_period(PWM1_TIM, new_arr);
                timer_set_oc_value(PWM1_TIM, PWM1_CH, new_arr / 2);
                
                // 2. Mantener el estado por 30 SEGUNDOS
                // Durante este tiempo, mantenemos el Mutex tomado.
                // Esto impide que task_control modifique el PWM.
                vTaskDelay(pdMS_TO_TICKS(30000));
                
                // 3. Liberar el recurso
                // Al soltar el Mutex, la tarea PID podrá volver a escribir en el timer
                // y restaurará la frecuencia según el potenciómetro en su siguiente ciclo.
                xSemaphoreGive(mutex_pwm_resource);
                
                // Limpiar cola de semáforos por si hubo rebotes del botón durante la espera
                xSemaphoreTake(sem_button_pressed, 0);
            }
        }
    }
}

void task_control(void *args) {
    (void)args;
    
    PID_Controller freq_pid;
    PID_Init(&freq_pid, 0.2f, 0.1f, 0.01f, 0.05f);
    
    const uint32_t TIM2_CLOCK = 1000000;
    float current_voltage = 0.0f;
    float target_freq = 10000.0f;
    
    if (sem_adc_ready == NULL || mutex_pwm_resource == NULL) {
        vTaskSuspend(NULL);
    }

    while (1) {
        adc_start_scan();

        if (xSemaphoreTake(sem_adc_ready, pdMS_TO_TICKS(100)) == pdTRUE) {
            
            if(xSemaphoreTake(mutex_adc_buffer, pdMS_TO_TICKS(10)) == pdTRUE) {
                current_voltage = (adc_dma_buffer[0] * 3.3f) / 4095.0f;
                xSemaphoreGive(mutex_adc_buffer);
            }

            if (current_voltage >= 1.8f && current_voltage <= 2.2f) {
                target_freq = 10000.0f;
            } else {
                target_freq = map_range(current_voltage, 0.0f, 3.3f, 1000.0f, 20000.0f);
            }

            freq_pid.setpoint = target_freq;
            
            // Si el botón Turbo tiene el Mutex, esta tarea se bloqueará aquí
            // esperando hasta 10ms. Si no lo consigue (porque está en turbo),
            // simplemente salta este ciclo y no toca el hardware.
            if (xSemaphoreTake(mutex_pwm_resource, pdMS_TO_TICKS(10)) == pdTRUE) {
                
                uint32_t current_arr = TIM_ARR(PWM1_TIM);
                float current_freq_hw = (float)TIM2_CLOCK / (float)(current_arr + 1);
                float error = target_freq - current_freq_hw;
                float adjustment_hz = PID_Calculate(&freq_pid, error);
                float final_freq = current_freq_hw + adjustment_hz;

                if (final_freq < 1000.0f) final_freq = 1000.0f;
                if (final_freq > 20000.0f) final_freq = 20000.0f;

                uint32_t new_period = (uint32_t)((TIM2_CLOCK / final_freq) - 1);
                
                timer_set_period(PWM1_TIM, new_period);
                timer_set_oc_value(PWM1_TIM, PWM1_CH, new_period / 2);
                
                xSemaphoreGive(mutex_pwm_resource);
            } else {
                // No pudimos tomar el control del PWM (Turbo activo).
                // Reseteamos el PID para evitar error acumulado (Windup) al volver
                PID_Reset(&freq_pid);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}