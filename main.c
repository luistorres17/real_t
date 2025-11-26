#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "config.h"
#include "app_task.h"

static void error_handler(void) {
    while (1) {
        gpio_toggle(LED_PORT, LED_PIN);
        for (volatile int i = 0; i < 100000; i++);
    }
}

int main(void) {
    BaseType_t task_status;
    
    clock_setup();
    gpio_setup();
    
    // 1. Crear primitivas
    sem_adc_ready = xSemaphoreCreateBinary();
    sem_button_pressed = xSemaphoreCreateBinary(); // Crear semáforo del botón
    mutex_adc_buffer = xSemaphoreCreateMutex();
    mutex_pwm_resource = xSemaphoreCreateMutex();
    
    if (!sem_adc_ready || !sem_button_pressed || !mutex_adc_buffer || !mutex_pwm_resource) {
        error_handler(); 
    }

    // 2. Configurar Periféricos
    dma_setup();
    adc_setup();
    pwm_setup();
    button_setup(); // Configurar EXTI del botón

    // 3. Crear Tareas
    // Tarea Blink (Prioridad 1 - Baja)
    xTaskCreate(task_blink, "BLINK", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    
    // Tarea Control PID (Prioridad 2 - Media)
    xTaskCreate(task_control, "CTRL", 256, NULL, 2, NULL);
    
    // Tarea Turbo (Prioridad 3 - Alta)
    // Debe ser mayor que CTRL para atender la interrupción rápido y bloquear el recurso
    task_status = xTaskCreate(task_turbo, "TURBO", 128, NULL, 3, NULL);
    if (task_status != pdPASS) error_handler();

    vTaskStartScheduler();

    error_handler();
    return 0;
}