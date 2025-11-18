#include "config.h"

// Buffer en memoria para los datos (0: Pot1, 1: Pot2)
volatile uint16_t adc_dma_buffer[2];

// Instancia del semáforo
SemaphoreHandle_t sem_adc_ready = NULL;

void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_DMA1); // ¡Importante!
}

void gpio_setup(void) {
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);
    gpio_set_mode(PWM_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, PWM1_PIN | PWM2_PIN);
    gpio_set_mode(ADC_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, POT1_PIN | POT2_PIN);
}

void dma_setup(void) {
    // ADC1 está conectado al DMA1 Canal 1 en el STM32F1
    dma_channel_reset(DMA1, DMA_CHANNEL1);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t)adc_dma_buffer);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 2); // 2 canales a leer
    
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL1);
    
    // --- CORRECCIÓN AQUÍ ---
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1); // Se añadió "_mode"
    // -----------------------

    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_16BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
    
    dma_enable_circular_mode(DMA1, DMA_CHANNEL1);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL1);
    
    dma_enable_channel(DMA1, DMA_CHANNEL1);
    
    // Habilitar interrupción en el NVIC
    nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    nvic_set_priority(NVIC_DMA1_CHANNEL1_IRQ, 5 * 16);
}

void adc_setup(void) {
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);
    adc_power_off(ADC_DEV);
    
    // Configuración SCAN (leer varios canales) + CONTINUOUS (no parar nunca)
    adc_enable_scan_mode(ADC_DEV);
    adc_set_continuous_conversion_mode(ADC_DEV);
    
    adc_set_single_conversion_mode(ADC_DEV); // (Nota: en scan+continuous esto se ignora a veces, pero es buena práctica dejarlo así para scan)
    adc_set_right_aligned(ADC_DEV);
    adc_set_sample_time_on_all_channels(ADC_DEV, ADC_SMPR_SMP_239DOT5CYC); // Lento y estable

    // Secuencia: Canal 1 (PA1) -> Canal 2 (PA2)
    uint8_t channels[] = { 1, 2 };
    adc_set_regular_sequence(ADC_DEV, 2, channels);

    // Habilitar petición DMA desde el ADC
    adc_enable_dma(ADC_DEV);

    adc_power_on(ADC_DEV);
    
    // Espera de estabilización
    for (int i = 0; i < 800000; i++) __asm__("nop");

    adc_reset_calibration(ADC_DEV);
    while (ADC_CR2(ADC_DEV) & ADC_CR2_RSTCAL);
    adc_calibrate(ADC_DEV);
    while (adc_is_calibrating(ADC_DEV));
    
    // Arrancamos la conversión UNA sola vez, el modo continuo + DMA hace el resto
    adc_start_conversion_regular(ADC_DEV);
}

void pwm_setup(void) {
    rcc_periph_reset_pulse(RST_TIM2);
    timer_set_mode(PWM_TIM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(PWM_TIM, 71);
    timer_set_period(PWM_TIM, 999);

    timer_set_oc_mode(PWM_TIM, PWM1_CH, TIM_OCM_PWM1);
    timer_enable_oc_output(PWM_TIM, PWM1_CH);
    
    timer_set_oc_mode(PWM_TIM, PWM2_CH, TIM_OCM_PWM1);
    timer_enable_oc_output(PWM_TIM, PWM2_CH);
    
    timer_enable_counter(PWM_TIM);
}

// --- INTERRUPCIÓN DEL DMA ---
// Esta función se llama automáticamente cuando el DMA llena el buffer
void dma1_channel1_isr(void) {
    // 1. Limpiar el flag de interrupción para que no se vuelva a disparar inmediatamente
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);

    // 2. Avisar a la tarea (dar el semáforo)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (sem_adc_ready != NULL) {
        xSemaphoreGiveFromISR(sem_adc_ready, &xHigherPriorityTaskWoken);
    }

    // 3. Si desbloqueamos una tarea de alta prioridad, forzamos cambio de contexto
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}