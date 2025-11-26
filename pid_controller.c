#include "pid_controller.h"

void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float dt) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->dt = dt;
    pid->setpoint = 0.0f; 
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    
    // Limites de corrección: El PID puede sugerir corregir hasta +/- 2000Hz por ciclo
    // Esto evita saltos demasiado bruscos
    pid->output_min = -2000.0f;
    pid->output_max = 2000.0f;
}

float PID_Calculate(PID_Controller *pid, float error) {
    // Término Proporcional
    float P = pid->Kp * error;
    
    // Término Integral
    pid->integral += error * pid->dt;
    float I = pid->Ki * pid->integral;
    
    // Término Derivativo
    float derivative = (error - pid->prev_error) / pid->dt;
    float D = pid->Kd * derivative;
    
    // Salida total (Ajuste de Hz)
    float output = P + I + D;
    
    // Anti-windup y Clamping
    if (output > pid->output_max) {
        output = pid->output_max;
        pid->integral -= error * pid->dt; // Evitar acumulación infinita
    } else if (output < pid->output_min) {
        output = pid->output_min;
        pid->integral -= error * pid->dt;
    }
    
    pid->prev_error = error;
    return output;
}

void PID_Reset(PID_Controller *pid) {
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}