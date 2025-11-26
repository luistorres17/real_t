#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>

typedef struct {
    float Kp;           // Ganancia Proporcional
    float Ki;           // Ganancia Integral
    float Kd;           // Ganancia Derivativa
    
    float setpoint;     // Valor objetivo
    float integral;     // Acumulador integral
    float prev_error;   // Error previo
    
    float output_min;   // Limite salida min (corrección Hz)
    float output_max;   // Limite salida max (corrección Hz)
    
    float dt;           // Tiempo de muestreo
} PID_Controller;

void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float dt);
float PID_Calculate(PID_Controller *pid, float error);
void PID_Reset(PID_Controller *pid);

#endif /* PID_CONTROLLER_H */