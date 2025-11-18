#include "config.h"

SemaphoreHandle_t sem_adc_ready = NULL;

void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_clock_enable(RCC_ADC1);
}

void gpio_setup(void) {
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);
    gpio_set_mode(PWM_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, PWM1_PIN | PWM2_PIN);
    gpio_set_mode(ADC_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, POT1_PIN | POT2_PIN);
}

void dma_setup(void) {
    // No usado en esta versión
}

void adc_setup(void) {
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6); // 12MHz
    adc_power_off(ADC_DEV);
    
    adc_disable_scan_mode(ADC_DEV);
    adc_set_single_conversion_mode(ADC_DEV);
    adc_set_right_aligned(ADC_DEV);
    adc_set_sample_time_on_all_channels(ADC_DEV, ADC_SMPR_SMP_239DOT5CYC);

    // --- CORRECCIÓN CRÍTICA ---
    // Habilitar el disparador externo y seleccionarlo como SWSTART.
    // Sin esto, adc_start_conversion_regular() no hace nada en el F1.
    adc_enable_external_trigger_regular(ADC_DEV, ADC_CR2_EXTSEL_SWSTART);
    // --------------------------

    adc_power_on(ADC_DEV);
    
    // Espera de estabilización (importante)
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

// Lectura bloqueante con Timeout
uint16_t adc_read_blocking(uint8_t channel) {
    uint8_t channel_array[16];
    channel_array[0] = channel;
    
    adc_set_regular_sequence(ADC_DEV, 1, channel_array);
    
    // Limpiar flag anterior por seguridad
    ADC_SR(ADC_DEV) = 0;
    
    // Iniciar conversión (Ahora sí funcionará por el trigger configurado)
    adc_start_conversion_regular(ADC_DEV);
    
    // Esperar EOC
    uint32_t timeout = 0xFFFFF; 
    while (!adc_eoc(ADC_DEV)) {
        if (--timeout == 0) {
            return 0xFFFF; // CÓDIGO DE ERROR: Devolvemos valor máximo (65535)
        }
    }
    
    return adc_read_regular(ADC_DEV);
}