#ifndef PID_H
#define PID_H

// PID控制结构体
typedef struct {
    double Kp;    // 比例系数
    double Ki;    // 积分系数
    double Kd;    // 微分系数
    double prev_error; // 上一个误差
    double integral;   // 积分值
} PID;


extern PID amplitude_pid[8], phase_pid[8];

double PID_adjust_amplitude(double setpoint_value, double actual_value, PID *amplitude_pid);
double PID_adjust_phase(double setpoint_value, double actual_value, PID *phase_pid);
void PID_Init_All();

#endif
