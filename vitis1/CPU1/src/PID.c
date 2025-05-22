#include "PID.h"
#include <stdio.h>
#include <math.h>

PID amplitude_pid[8], phase_pid[8];
// 初始化PID控制器
void PID_Init(PID *pid, double Kp, double Ki, double Kd)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->prev_error = 0;
    pid->integral = 0;
    pid->last_valid_output = 0;
}

// 初始化8个PID控制器
void PID_Init_All()
{
    // 初始化幅值PID
    for (int i = 0; i < 8; i++)
    {
        // 对于每个幅值PID，假设你有一些默认的 Kp, Ki, Kd 参数
        double Kp_amplitude = 0.2, Ki_amplitude = 0.02, Kd_amplitude = 0.0;
        PID_Init(&amplitude_pid[i], Kp_amplitude, Ki_amplitude, Kd_amplitude);
    }

    // 初始化相位PID
    for (int i = 0; i < 8; i++)
    {
        // 对于每个相位PID，假设你有一些默认的 Kp, Ki, Kd 参数
        double Kp_phase = 0.5, Ki_phase = 0.0, Kd_phase = 0.0;
        PID_Init(&phase_pid[i], Kp_phase, Ki_phase, Kd_phase);
    }
}

// PID计算函数
double PID_Compute(PID *pid, double setpoint, double actual_value)
{
    double error = setpoint - actual_value;      // 计算误差
    pid->integral += error;                      // 积分部分
    double derivative = error - pid->prev_error; // 微分部分
    pid->prev_error = error;                     // 更新上一个误差

    // PID输出
    return pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;
}

// 调整幅值的函数// 调整幅值的函数
double PID_adjust_amplitude(double setpoint_value, double actual_value, PID *amplitude_pid)
{
    // 打印setpoint_value和actual_value
    printf("Setpoint Value: %f, Actual Value: %f\n", setpoint_value, actual_value);
    //  防止除以零
    if (fabs(setpoint_value) < 1e-6)
    {
        return 0; // 如果设定值接近零，不进行调整
    }

    // 计算幅值误差
    double absolute_error = fabs(setpoint_value - actual_value);
    double relative_error = absolute_error / fabs(setpoint_value);
    const double MAX_INTEGRAL = 1.0;
    // 定义幅值误差的上限阈值（例如20%）
    const double AMPLITUDE_ERROR_MAX_THRESHOLD = 0.20;
    // 定义幅值误差的可接受范围，定义万分之一的精度
    const double AMPLITUDE_ERROR_MIN_THRESHOLD = 0.0001;

    // 如果误差太大，不进行调整以保护设备
    if (relative_error > AMPLITUDE_ERROR_MAX_THRESHOLD)
    {
        // 重置积分项以防止积分饱和
        amplitude_pid->integral = 0;
        amplitude_pid->last_valid_output = 0; // 误差过大，之前的PID输出可能不再适用
        printf("Error too large, PID output set to 0.\n");
        return 0;
    }
    // 如果误差大于最小阈值但小于最大阈值，执行PID调整
    if (relative_error > AMPLITUDE_ERROR_MIN_THRESHOLD)
    {
        // 防止积分饱和
        if (amplitude_pid->integral > MAX_INTEGRAL)
        {
            amplitude_pid->integral = MAX_INTEGRAL;
        }
        else if (amplitude_pid->integral < -MAX_INTEGRAL)
        {
            amplitude_pid->integral = -MAX_INTEGRAL;
        }
        // 计算新的PID输出并保存
        amplitude_pid->last_valid_output = PID_Compute(amplitude_pid, setpoint_value, actual_value);
        printf("PID adjusting, output: %f\n", amplitude_pid->last_valid_output);
        return amplitude_pid->last_valid_output;
    }

    // 如果误差很小，不进行调整
    // 此时不调用PID_Compute，而是返回上一次有效的PID输出值
    printf("Error within tolerance, maintaining last valid PID output: %f\n", amplitude_pid->last_valid_output);
    // 清除积分项，防止在目标值附近时积分持续累积导致震荡或超调
    // amplitude_pid->integral = 0;
    return amplitude_pid->last_valid_output;
}

// 调整相位的函数
double PID_adjust_phase(double setpoint_value, double actual_value, PID *phase_pid)
{
    // 将角度标准化到[0, 360)范围
    setpoint_value = fmod(setpoint_value, 360.0);
    if (setpoint_value < 0)
        setpoint_value += 360.0;

    actual_value = fmod(actual_value, 360.0);
    if (actual_value < 0)
        actual_value += 360.0;

    // 计算相位差，考虑最短路径
    double phase_diff = setpoint_value - actual_value;

    // 确保我们沿着圆周取最短路径
    if (phase_diff > 180.0)
    {
        phase_diff -= 360.0;
    }
    else if (phase_diff < -180.0)
    {
        phase_diff += 360.0;
    }

    // 定义相位误差的上限阈值（例如10度）
    const double PHASE_ERROR_MAX_THRESHOLD_DEGREES = 10.0;

    // 如果误差太大，不进行调整
    if (fabs(phase_diff) > PHASE_ERROR_MAX_THRESHOLD_DEGREES)
    {
        // 重置积分项以防止积分饱和
        phase_pid->integral = 0;
        return 0;
    }

    // 定义相位误差的最小阈值（0.1度）
    const double PHASE_ERROR_MIN_THRESHOLD_DEGREES = 0.01;

    // 如果误差足够大但不太大，执行PID调整
    if (fabs(phase_diff) > PHASE_ERROR_MIN_THRESHOLD_DEGREES)
    {
        // 积分项添加限幅
        phase_pid->integral += phase_diff;

        // 积分限幅，防止积分饱和
        const double MAX_INTEGRAL = 50.0;
        if (phase_pid->integral > MAX_INTEGRAL)
            phase_pid->integral = MAX_INTEGRAL;
        else if (phase_pid->integral < -MAX_INTEGRAL)
            phase_pid->integral = -MAX_INTEGRAL;

        double derivative = phase_diff - phase_pid->prev_error;
        phase_pid->prev_error = phase_diff;

        return phase_pid->Kp * phase_diff + phase_pid->Ki * phase_pid->integral + phase_pid->Kd * derivative;
    }

    // 如果误差很小，不进行调整
    return 0;
}