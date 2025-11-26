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

// --- Hardware ---
#define LED_PORT    GPIOC
#define LED_PIN     GPIO13

// --- PWM 1: Frecuencia Variable ---
// Usa Timer 2, Canal 1
#define PWM1_PORT   GPIOA
#define PWM1_PIN    GPIO0
#define PWM1_TIM    TIM2
#define PWM1_CH     TIM_OC1

// --- ADC ---
#define ADC_PORT    GPIOA
#define POT1_PIN    GPIO1
#define ADC_DEV     ADC1

// --- Globales ---
extern volatile uint16_t adc_dma_buffer[1];
extern SemaphoreHandle_t sem_adc_ready;
extern SemaphoreHandle_t mutex_adc_buffer;

// --- Funciones ---
void clock_setup(void);
void gpio_setup(void);
void dma_setup(void);
void adc_setup(void);
void pwm_setup(void);
void adc_start_scan(void);

#endif /* CONFIG_H */