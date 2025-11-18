#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "config.h"
#include "app_task.h"

int main(void) {
    clock_setup();
    gpio_setup();
    
    // Crear semáforo antes de configurar interrupciones
    sem_adc_ready = xSemaphoreCreateBinary();

    dma_setup();
    adc_setup();
    pwm_setup();

    xTaskCreate(task_blink, "BLINK", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    
    // Stack generoso para no arriesgar
    xTaskCreate(task_control, "CTRL", 256, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1);
    return 0;
}