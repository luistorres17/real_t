#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>

typedef struct {
    float Kp;           // Proportional gain
    float Ki;           // Integral gain
    float Kd;           // Derivative gain
    
    float setpoint;     // Target value (10000 Hz)
    float integral;     // Integral accumulator
    float prev_error;   // Previous error for derivative
    
    float output_min;   // Anti-windup: minimum output (period)
    float output_max;   // Anti-windup: maximum output (period)
    
    float dt;           // Sample time (seconds)
} PID_Controller;

// Initialize PID controller
void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float dt);

// Calculate PID output
float PID_Calculate(PID_Controller *pid, float error);

// Reset PID state
void PID_Reset(PID_Controller *pid);

#endif /* PID_CONTROLLER_H */
