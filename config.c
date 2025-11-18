#include "config.h"

// Buffer de 1 sola variable
volatile uint16_t adc_val = 0;
SemaphoreHandle_t sem_adc_ready = NULL;

void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_DMA1);
}

void gpio_setup(void) {
    // LED (PC13)
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);

    // PWM (PA0) - Alternate Function Push-Pull
    gpio_set_mode(PWM_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, PWM_PIN);

    // ADC (PA1) - Analog Input
    gpio_set_mode(ADC_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, POT_PIN);
}

void dma_setup(void) {
    // Usamos DMA1 Canal 1 (Canal por defecto para ADC1)
    dma_channel_reset(DMA1, DMA_CHANNEL1);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t)&adc_val);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 1); // Solo 1 dato
    
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL1);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1); // (Opcional para 1 dato, pero seguro)
    
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_16BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
    
    // Sin modo circular. Queremos control total.
    
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL1);
    nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    nvic_set_priority(NVIC_DMA1_CHANNEL1_IRQ, 5 * 16);
}

void adc_setup(void) {
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6); // 12MHz ADC Clock
    adc_power_off(ADC_DEV);
    
    // Configuración más simple posible:
    adc_disable_scan_mode(ADC_DEV);           // Solo 1 canal
    adc_set_single_conversion_mode(ADC_DEV);  // 1 conversión y para
    adc_set_right_aligned(ADC_DEV);
    adc_set_sample_time_on_all_channels(ADC_DEV, ADC_SMPR_SMP_239DOT5CYC);

    // Secuencia: Leer solo canal 1 (PA1)
    uint8_t channels[] = { 1 };
    adc_set_regular_sequence(ADC_DEV, 1, channels);

    adc_enable_dma(ADC_DEV);
    adc_power_on(ADC_DEV);
    
    // Espera de estabilización
    for (int i = 0; i < 800000; i++) __asm__("nop");

    adc_reset_calibration(ADC_DEV);
    while (ADC_CR2(ADC_DEV) & ADC_CR2_RSTCAL);
    adc_calibrate(ADC_DEV);
    while (adc_is_calibrating(ADC_DEV));
}

void pwm_setup(void) {
    rcc_periph_reset_pulse(RST_TIM2);
    
    // Timer a 1 kHz
    timer_set_mode(PWM_TIM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(PWM_TIM, 71);      // 1MHz tick
    timer_set_period(PWM_TIM, 999);        // 1ms periodo

    // Canal 1 (PA0)
    timer_set_oc_mode(PWM_TIM, PWM_CH, TIM_OCM_PWM1);
    timer_enable_oc_output(PWM_TIM, PWM_CH);
    timer_set_oc_value(PWM_TIM, PWM_CH, 0); // Empezar apagado

    timer_enable_counter(PWM_TIM);
}

// Función para disparar UNA lectura
void adc_read_single(void) {
    dma_disable_channel(DMA1, DMA_CHANNEL1);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 1); // Resetear contador a 1
    dma_enable_channel(DMA1, DMA_CHANNEL1);
    
    adc_start_conversion_regular(ADC_DEV); // Disparo manual
}

// ISR: Se llama cuando el DMA termina de copiar el dato a la RAM
void dma1_channel1_isr(void) {
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (sem_adc_ready != NULL) {
        xSemaphoreGiveFromISR(sem_adc_ready, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}