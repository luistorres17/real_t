#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "config.h"
#include "app_task.h"

// Función de error: parpadeo rápido del LED
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
    
    // Crear semáforo antes de configurar interrupciones
    sem_adc_ready = xSemaphoreCreateBinary();
    if (sem_adc_ready == NULL) {
        error_handler();  // Fallo crítico: no se pudo crear el semáforo
    }
    
    // Crear mutex para proteger acceso al buffer DMA
    mutex_adc_buffer = xSemaphoreCreateMutex();
    if (mutex_adc_buffer == NULL) {
        error_handler();  // Fallo crítico: no se pudo crear el mutex
    }

    dma_setup();
    adc_setup();
    pwm_setup();

    // Crear tarea de parpadeo con validación
    task_status = xTaskCreate(task_blink, "BLINK", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    if (task_status != pdPASS) {
        error_handler();  // Fallo: no se pudo crear task_blink
    }
    
    // Stack reducido de 256 a 128 palabras (suficiente para la lógica actual)
    task_status = xTaskCreate(task_control, "CTRL", 128, NULL, 2, NULL);
    if (task_status != pdPASS) {
        error_handler();  // Fallo: no se pudo crear task_control
    }

    vTaskStartScheduler();

    // Si llegamos aquí, el scheduler falló (no debería pasar)
    error_handler();
    
    return 0;
}