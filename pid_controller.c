#include "pid_controller.h"

void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float dt) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->dt = dt;
    pid->setpoint = 10000.0f;  // 10 kHz fixed setpoint
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    
    // Output limits: timer period values
    // For 1 MHz timer clock:
    // 20 kHz -> period = 50
    // 1 kHz -> period = 1000
    pid->output_min = 50.0f;    // Max frequency (20 kHz)
    pid->output_max = 1000.0f;  // Min frequency (1 kHz)
}

float PID_Calculate(PID_Controller *pid, float error) {
    // Proportional term
    float P = pid->Kp * error;
    
    // Integral term
    pid->integral += error * pid->dt;
    float I = pid->Ki * pid->integral;
    
    // Derivative term
    float derivative = (error - pid->prev_error) / pid->dt;
    float D = pid->Kd * derivative;
    
    // Calculate total output
    float output = P + I + D;
    
    // Anti-windup: clamp output and back-calculate integral
    if (output > pid->output_max) {
        output = pid->output_max;
        // Back-calculate integral to prevent windup
        pid->integral -= error * pid->dt;
    } else if (output < pid->output_min) {
        output = pid->output_min;
        // Back-calculate integral to prevent windup
        pid->integral -= error * pid->dt;
    }
    
    // Store error for next iteration
    pid->prev_error = error;
    
    return output;
}

void PID_Reset(PID_Controller *pid) {
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}
