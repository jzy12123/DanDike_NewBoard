#ifndef PID_H
#define PID_H

// PID���ƽṹ��
typedef struct {
    double Kp;    // ����ϵ��
    double Ki;    // ����ϵ��
    double Kd;    // ΢��ϵ��
    double prev_error; // ��һ�����
    double integral;   // ����ֵ
} PID;


extern PID amplitude_pid[8], phase_pid[8];

double PID_adjust_amplitude(double setpoint_value, double actual_value, PID *amplitude_pid);
double PID_adjust_phase(double setpoint_value, double actual_value, PID *phase_pid);
void PID_Init_All();

#endif
