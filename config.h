#ifndef CONFIG_H
#define CONFIG_H

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h> // Necesario para interrupciones externas
#include "FreeRTOS.h"
#include "semphr.h"

// --- Hardware ---
#define LED_PORT    GPIOC
#define LED_PIN     GPIO13

// --- Botón (Interrupción) ---
// Usaremos PB5 para no conflictuar con PA0/PA1 en las lineas EXTI
#define BUTTON_PORT GPIOB
#define BUTTON_PIN  GPIO5
#define BUTTON_EXTI EXTI5
#define BUTTON_NVIC NVIC_EXTI9_5_IRQ

// --- PWM 1: Frecuencia Variable ---
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
extern SemaphoreHandle_t mutex_pwm_resource;
extern SemaphoreHandle_t sem_button_pressed; // Nuevo semáforo para el botón

// --- Funciones ---
void clock_setup(void);
void gpio_setup(void);
void button_setup(void); // Nueva función
void dma_setup(void);
void adc_setup(void);
void pwm_setup(void);
void adc_start_scan(void);

#endif /* CONFIG_H */