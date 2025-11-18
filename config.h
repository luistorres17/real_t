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

// --- LED de Estado (PC13) ---
#define LED_PORT    GPIOC
#define LED_PIN     GPIO13

// --- PWM (Solo PA0) ---
#define PWM_PORT    GPIOA
#define PWM_TIM     TIM2
#define PWM_PIN     GPIO0
#define PWM_CH      TIM_OC1

// --- ADC (Solo PA1) ---
#define ADC_PORT    GPIOA
#define POT_PIN     GPIO1
#define ADC_DEV     ADC1

// --- Variables Globales ---
// Buffer para 1 solo valor (16 bits)
extern volatile uint16_t adc_val;
extern SemaphoreHandle_t sem_adc_ready;

// --- Funciones ---
void clock_setup(void);
void gpio_setup(void);
void dma_setup(void);
void adc_setup(void);
void pwm_setup(void);
void adc_read_single(void); // Función simplificada

#endif /* CONFIG_H */