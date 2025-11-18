#include "config.h"

// Buffer para los datos del ADC
volatile uint16_t adc_dma_buffer[2];
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
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);
    gpio_set_mode(PWM_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, PWM1_PIN | PWM2_PIN);
    gpio_set_mode(ADC_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, POT1_PIN | POT2_PIN);
}

void dma_setup(void) {
    // DMA1 Canal 1 sirve al ADC1
    dma_channel_reset(DMA1, DMA_CHANNEL1);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t)adc_dma_buffer);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 2);
    
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL1);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_16BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
    
    // IMPORTANTE: Sin modo circular. Queremos disparar -> leer 2 datos -> parar.
    // dma_enable_circular_mode(DMA1, DMA_CHANNEL1);
    
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL1);
    
    nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    nvic_set_priority(NVIC_DMA1_CHANNEL1_IRQ, 5 * 16);
}

void adc_setup(void) {
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);
    adc_power_off(ADC_DEV);
    
    // Configuración SCAN (Multi-canal)
    adc_enable_scan_mode(ADC_DEV);
    adc_set_single_conversion_mode(ADC_DEV);
    adc_set_right_aligned(ADC_DEV);
    adc_set_sample_time_on_all_channels(ADC_DEV, ADC_SMPR_SMP_239DOT5CYC);

    uint8_t channels[] = { 1, 2 }; // PA1, PA2
    adc_set_regular_sequence(ADC_DEV, 2, channels);

    // --- EL FIX CRÍTICO ---
    // Habilitar trigger externo para SWSTART. Sin esto, el F1 no arranca por software.
    adc_enable_external_trigger_regular(ADC_DEV, ADC_CR2_EXTSEL_SWSTART);
    // ---------------------

    // Habilitar DMA en el ADC
    adc_enable_dma(ADC_DEV);

    adc_power_on(ADC_DEV);
    
    for (int i = 0; i < 800000; i++) __asm__("nop");

    adc_reset_calibration(ADC_DEV);
    while (ADC_CR2(ADC_DEV) & ADC_CR2_RSTCAL);
    adc_calibrate(ADC_DEV);
    while (adc_is_calibrating(ADC_DEV));
}

void pwm_setup(void) {
    rcc_periph_reset_pulse(RST_TIM2);
    timer_set_mode(PWM_TIM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(PWM_TIM, 71);
    timer_set_period(PWM_TIM, 999);

    timer_set_oc_mode(PWM_TIM, PWM1_CH, TIM_OCM_PWM1);
    timer_enable_oc_output(PWM_TIM, PWM1_CH);
    timer_set_oc_value(PWM_TIM, PWM1_CH, 0);

    timer_set_oc_mode(PWM_TIM, PWM2_CH, TIM_OCM_PWM1);
    timer_enable_oc_output(PWM_TIM, PWM2_CH);
    timer_set_oc_value(PWM_TIM, PWM2_CH, 0);

    timer_enable_counter(PWM_TIM);
}

// Función para rearmar y disparar
void adc_start_scan(void) {
    // 1. Apagar DMA para recargar contador
    dma_disable_channel(DMA1, DMA_CHANNEL1);
    
    // 2. Limpiar banderas viejas
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);
    ADC_SR(ADC_DEV) = 0;

    // 3. Recargar contador a 2 conversiones
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 2);
    
    // 4. Habilitar DMA y Disparar ADC
    dma_enable_channel(DMA1, DMA_CHANNEL1);
    adc_start_conversion_regular(ADC_DEV);
}

// ISR del DMA: Avisa a la tarea cuando terminó
void dma1_channel1_isr(void) {
    if (dma_get_interrupt_flag(DMA1, DMA_CHANNEL1, DMA_TCIF)) {
        dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (sem_adc_ready != NULL) {
            xSemaphoreGiveFromISR(sem_adc_ready, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}