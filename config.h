#ifndef CONFIG_H
#define CONFIG_H

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include "FreeRTOS.h"
#include "semphr.h"

// --- Hardware ---
#define LED_PORT    GPIOC
#define LED_PIN     GPIO13

// PWM
#define PWM_PORT    GPIOA
#define PWM_TIM     TIM2
#define PWM1_PIN    GPIO0
#define PWM1_CH     TIM_OC1
#define PWM2_PIN    GPIO3
#define PWM2_CH     TIM_OC4

// ADC
#define ADC_PORT    GPIOA
#define POT1_PIN    GPIO1
#define POT2_PIN    GPIO2
#define ADC_DEV     ADC1

// --- Globales ---
// Mantenemos el semáforo solo para que main.c no falle al compilar
extern SemaphoreHandle_t sem_adc_ready;

// --- Funciones ---
void clock_setup(void);
void gpio_setup(void);
void dma_setup(void); // Se mantiene vacía para compatibilidad
void adc_setup(void);
void pwm_setup(void);

// NUEVA: Función que devuelve el valor inmediatamente
uint16_t adc_read_blocking(uint8_t channel);

#endif /* CONFIG_H */