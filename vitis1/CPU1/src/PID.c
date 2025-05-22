#include "PID.h"
#include <stdio.h>
#include <math.h>

PID amplitude_pid[8], phase_pid[8];
// ��ʼ��PID������
void PID_Init(PID *pid, double Kp, double Ki, double Kd)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->prev_error = 0;
    pid->integral = 0;
    pid->last_valid_output = 0;
}

// ��ʼ��8��PID������
void PID_Init_All()
{
    // ��ʼ����ֵPID
    for (int i = 0; i < 8; i++)
    {
        // ����ÿ����ֵPID����������һЩĬ�ϵ� Kp, Ki, Kd ����
        double Kp_amplitude = 0.2, Ki_amplitude = 0.02, Kd_amplitude = 0.0;
        PID_Init(&amplitude_pid[i], Kp_amplitude, Ki_amplitude, Kd_amplitude);
    }

    // ��ʼ����λPID
    for (int i = 0; i < 8; i++)
    {
        // ����ÿ����λPID����������һЩĬ�ϵ� Kp, Ki, Kd ����
        double Kp_phase = 0.5, Ki_phase = 0.0, Kd_phase = 0.0;
        PID_Init(&phase_pid[i], Kp_phase, Ki_phase, Kd_phase);
    }
}

// PID���㺯��
double PID_Compute(PID *pid, double setpoint, double actual_value)
{
    double error = setpoint - actual_value;      // �������
    pid->integral += error;                      // ���ֲ���
    double derivative = error - pid->prev_error; // ΢�ֲ���
    pid->prev_error = error;                     // ������һ�����

    // PID���
    return pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;
}

// ������ֵ�ĺ���// ������ֵ�ĺ���
double PID_adjust_amplitude(double setpoint_value, double actual_value, PID *amplitude_pid)
{
    // ��ӡsetpoint_value��actual_value
    printf("Setpoint Value: %f, Actual Value: %f\n", setpoint_value, actual_value);
    //  ��ֹ������
    if (fabs(setpoint_value) < 1e-6)
    {
        return 0; // ����趨ֵ�ӽ��㣬�����е���
    }

    // �����ֵ���
    double absolute_error = fabs(setpoint_value - actual_value);
    double relative_error = absolute_error / fabs(setpoint_value);
    const double MAX_INTEGRAL = 1.0;
    // �����ֵ����������ֵ������20%��
    const double AMPLITUDE_ERROR_MAX_THRESHOLD = 0.20;
    // �����ֵ���Ŀɽ��ܷ�Χ���������֮һ�ľ���
    const double AMPLITUDE_ERROR_MIN_THRESHOLD = 0.0001;

    // ������̫�󣬲����е����Ա����豸
    if (relative_error > AMPLITUDE_ERROR_MAX_THRESHOLD)
    {
        // ���û������Է�ֹ���ֱ���
        amplitude_pid->integral = 0;
        amplitude_pid->last_valid_output = 0; // ������֮ǰ��PID������ܲ�������
        printf("Error too large, PID output set to 0.\n");
        return 0;
    }
    // �����������С��ֵ��С�������ֵ��ִ��PID����
    if (relative_error > AMPLITUDE_ERROR_MIN_THRESHOLD)
    {
        // ��ֹ���ֱ���
        if (amplitude_pid->integral > MAX_INTEGRAL)
        {
            amplitude_pid->integral = MAX_INTEGRAL;
        }
        else if (amplitude_pid->integral < -MAX_INTEGRAL)
        {
            amplitude_pid->integral = -MAX_INTEGRAL;
        }
        // �����µ�PID���������
        amplitude_pid->last_valid_output = PID_Compute(amplitude_pid, setpoint_value, actual_value);
        printf("PID adjusting, output: %f\n", amplitude_pid->last_valid_output);
        return amplitude_pid->last_valid_output;
    }

    // �������С�������е���
    // ��ʱ������PID_Compute�����Ƿ�����һ����Ч��PID���ֵ
    printf("Error within tolerance, maintaining last valid PID output: %f\n", amplitude_pid->last_valid_output);
    // ����������ֹ��Ŀ��ֵ����ʱ���ֳ����ۻ������𵴻򳬵�
    // amplitude_pid->integral = 0;
    return amplitude_pid->last_valid_output;
}

// ������λ�ĺ���
double PID_adjust_phase(double setpoint_value, double actual_value, PID *phase_pid)
{
    // ���Ƕȱ�׼����[0, 360)��Χ
    setpoint_value = fmod(setpoint_value, 360.0);
    if (setpoint_value < 0)
        setpoint_value += 360.0;

    actual_value = fmod(actual_value, 360.0);
    if (actual_value < 0)
        actual_value += 360.0;

    // ������λ��������·��
    double phase_diff = setpoint_value - actual_value;

    // ȷ����������Բ��ȡ���·��
    if (phase_diff > 180.0)
    {
        phase_diff -= 360.0;
    }
    else if (phase_diff < -180.0)
    {
        phase_diff += 360.0;
    }

    // ������λ����������ֵ������10�ȣ�
    const double PHASE_ERROR_MAX_THRESHOLD_DEGREES = 10.0;

    // ������̫�󣬲����е���
    if (fabs(phase_diff) > PHASE_ERROR_MAX_THRESHOLD_DEGREES)
    {
        // ���û������Է�ֹ���ֱ���
        phase_pid->integral = 0;
        return 0;
    }

    // ������λ������С��ֵ��0.1�ȣ�
    const double PHASE_ERROR_MIN_THRESHOLD_DEGREES = 0.01;

    // �������㹻�󵫲�̫��ִ��PID����
    if (fabs(phase_diff) > PHASE_ERROR_MIN_THRESHOLD_DEGREES)
    {
        // ����������޷�
        phase_pid->integral += phase_diff;

        // �����޷�����ֹ���ֱ���
        const double MAX_INTEGRAL = 50.0;
        if (phase_pid->integral > MAX_INTEGRAL)
            phase_pid->integral = MAX_INTEGRAL;
        else if (phase_pid->integral < -MAX_INTEGRAL)
            phase_pid->integral = -MAX_INTEGRAL;

        double derivative = phase_diff - phase_pid->prev_error;
        phase_pid->prev_error = phase_diff;

        return phase_pid->Kp * phase_diff + phase_pid->Ki * phase_pid->integral + phase_pid->Kd * derivative;
    }

    // �������С�������е���
    return 0;
}