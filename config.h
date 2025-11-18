#ifndef CONFIG_H
#define CONFIG_H

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include "FreeRTOS.h"
#include "semphr.h"

// --- Periféricos ---
#define LED_PORT    GPIOC
#define LED_PIN     GPIO13

#define PWM_PORT    GPIOA
#define PWM_TIM     TIM2
#define PWM1_PIN    GPIO0
#define PWM1_CH     TIM_OC1
#define PWM2_PIN    GPIO3
#define PWM2_CH     TIM_OC4

#define ADC_PORT    GPIOA
#define POT1_PIN    GPIO1
#define POT2_PIN    GPIO2
#define ADC_DEV     ADC1

// --- Globales ---
// Buffer para los 2 canales del ADC (Pot1 y Pot2)
extern volatile uint16_t adc_dma_buffer[2];
// Semáforo para sincronizar DMA -> Tarea
extern SemaphoreHandle_t sem_adc_ready;

// --- Funciones ---
void clock_setup(void);
void gpio_setup(void);
void pwm_setup(void);
void dma_setup(void);
void adc_setup(void);

#endif /* CONFIG_H */