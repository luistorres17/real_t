#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" // Necesario para crear el semáforo
#include "config.h"
#include "app_task.h"

int main(void) {
    clock_setup();
    gpio_setup();
    
    // Crear el semáforo binario antes de usarlo en interrupciones o tareas
    sem_adc_ready = xSemaphoreCreateBinary();

    // Orden correcto: Configurar DMA -> Configurar ADC (que arranca el dma)
    dma_setup();
    adc_setup();
    pwm_setup();

    xTaskCreate(task_blink, "BLINK", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    
    // Prioridad más alta para el control
    xTaskCreate(task_control, "CTRL", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1);
    return 0;
}