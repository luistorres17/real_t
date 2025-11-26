#include "config.h"

volatile uint16_t adc_dma_buffer[1];
SemaphoreHandle_t sem_adc_ready = NULL;
SemaphoreHandle_t mutex_adc_buffer = NULL;
SemaphoreHandle_t mutex_pwm_resource = NULL;
SemaphoreHandle_t sem_button_pressed = NULL; // Instancia del semáforo

void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB); // Reloj para el puerto del botón
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_DMA1);
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_clock_enable(RCC_AFIO);  // Necesario para EXTI
}

void gpio_setup(void) {
    gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, LED_PIN);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, PWM1_PIN);
    gpio_set_mode(ADC_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, POT1_PIN);
}

// Configuración de la interrupción del botón
void button_setup(void) {
    // Configurar pin como entrada con Pull-Up (asumiendo botón conecta a GND)
    gpio_set_mode(BUTTON_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, BUTTON_PIN);
    gpio_set(BUTTON_PORT, BUTTON_PIN); // Activar Pull-Up interno

    // Configurar EXTI (External Interrupt)
    nvic_enable_irq(BUTTON_NVIC);
    nvic_set_priority(BUTTON_NVIC, 6 * 16); // Prioridad intermedia

    // Mapear PB5 a la linea EXTI5
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, 0); // Asegurar SWD activo
    exti_select_source(BUTTON_EXTI, BUTTON_PORT);
    exti_set_trigger(BUTTON_EXTI, EXTI_TRIGGER_FALLING); // Flanco de bajada (al presionar)
    exti_enable_request(BUTTON_EXTI);
}

void dma_setup(void) {
    dma_channel_reset(DMA1, DMA_CHANNEL1);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL1, (uint32_t)&ADC_DR(ADC1));
    dma_set_memory_address(DMA1, DMA_CHANNEL1, (uint32_t)adc_dma_buffer);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 1);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL1);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_16BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL1);
    nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
    nvic_set_priority(NVIC_DMA1_CHANNEL1_IRQ, 5 * 16);
}

void adc_setup(void) {
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);
    adc_power_off(ADC_DEV);
    adc_disable_scan_mode(ADC_DEV);
    adc_set_single_conversion_mode(ADC_DEV);
    adc_set_right_aligned(ADC_DEV);
    adc_set_sample_time_on_all_channels(ADC_DEV, ADC_SMPR_SMP_239DOT5CYC);
    uint8_t channels[] = { 1 };
    adc_set_regular_sequence(ADC_DEV, 1, channels);
    adc_enable_external_trigger_regular(ADC_DEV, ADC_CR2_EXTSEL_SWSTART);
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
    timer_set_mode(PWM1_TIM, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(PWM1_TIM, 71); 
    timer_set_period(PWM1_TIM, 99); 
    timer_set_oc_mode(PWM1_TIM, PWM1_CH, TIM_OCM_PWM1);
    timer_enable_oc_output(PWM1_TIM, PWM1_CH);
    timer_set_oc_value(PWM1_TIM, PWM1_CH, 50);  
    timer_enable_counter(PWM1_TIM);
}

void adc_start_scan(void) {
    dma_disable_channel(DMA1, DMA_CHANNEL1);
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);
    dma_set_number_of_data(DMA1, DMA_CHANNEL1, 1);
    dma_enable_channel(DMA1, DMA_CHANNEL1);
    ADC_SR(ADC_DEV) = 0;
    adc_start_conversion_regular(ADC_DEV);
}

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

// Manejador de la interrupción del botón (EXTI Lines 5-9)
void exti9_5_isr(void) {
    if (exti_get_flag_status(BUTTON_EXTI)) {
        exti_reset_request(BUTTON_EXTI); // Limpiar bandera hardware
        
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (sem_button_pressed != NULL) {
            // Notificar a la tarea turbo
            xSemaphoreGiveFromISR(sem_button_pressed, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}