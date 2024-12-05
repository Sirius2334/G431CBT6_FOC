#include "openloop.h"

#include <math.h>

// 初始变量及函数定义
#define _constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
// 宏定义实现的一个约束函数,用于限制一个值的范围。
// 具体来说，该宏定义的名称为 _constrain，接受三个参数 amt、low 和 high，分别表示要限制的值、最小值和最大值。该宏定义的实现使用了三元运算符，根据 amt 是否小于 low 或大于 high，返回其中的最大或最小值，或者返回原值。
// 换句话说，如果 amt 小于 low，则返回 low；如果 amt 大于 high，则返回 high；否则返回 amt。这样，_constrain(amt, low, high) 就会将 amt 约束在 [low, high] 的范围内。
float voltage_power_supply = 12.6;
float shaft_angle = 0, open_loop_timestamp = 0;
float zero_electric_angle = 0, Ualpha, Ubeta = 0, Ua = 0, Ub = 0, Uc = 0, dc_a = 0, dc_b = 0, dc_c = 0;

__STATIC_INLINE uint32_t GXT_SYSTICK_IsActiveCounterFlag(void)
{
    return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == (SysTick_CTRL_COUNTFLAG_Msk));
}
static uint32_t getCurrentMicros(void)
{
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    GXT_SYSTICK_IsActiveCounterFlag();
    uint32_t m = HAL_GetTick();
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u = tms - SysTick->VAL;
    if (GXT_SYSTICK_IsActiveCounterFlag())
    {
        m = HAL_GetTick();
        u = tms - SysTick->VAL;
    }
    return (m * 1000 + (u * 1000) / tms);
}
// 获取系统时间，单位us
uint32_t micros(void)
{
    return getCurrentMicros();
}

// 电角度求解
float _electricalAngle(float shaft_angle, int pole_pairs)
{
    return (shaft_angle * pole_pairs);
}

// 归一化角度到 [0,2PI]
float _normalizeAngle(float angle)
{
    float a = fmod(angle, 2 * PI); // 取余运算可以用于归一化，列出特殊值例子算便知
    return a >= 0 ? a : (a + 2 * PI);
    // 三目运算符。格式：condition ? expr1 : expr2
    // 其中，condition 是要求值的条件表达式，如果条件成立，则返回 expr1 的值，否则返回 expr2 的值。可以将三目运算符视为 if-else 语句的简化形式。
    // fmod 函数的余数的符号与除数相同。因此，当 angle 的值为负数时，余数的符号将与 _2PI 的符号相反。也就是说，如果 angle 的值小于 0 且 _2PI 的值为正数，则 fmod(angle, _2PI) 的余数将为负数。
    // 例如，当 angle 的值为 -PI/2，_2PI 的值为 2PI 时，fmod(angle, _2PI) 将返回一个负数。在这种情况下，可以通过将负数的余数加上 _2PI 来将角度归一化到 [0, 2PI] 的范围内，以确保角度的值始终为正数。
}

// 设置PWM到控制器输出
void setPwm(float Ua, float Ub, float Uc)
{
    // 计算占空比
    // 限制占空比从0到1
    dc_a = _constrain(Ua / voltage_power_supply, 0.0f, 1.0f);
    dc_b = _constrain(Ub / voltage_power_supply, 0.0f, 1.0f);
    dc_c = _constrain(Uc / voltage_power_supply, 0.0f, 1.0f);

    // 写入PWM到PWM 0 1 2 通道
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, dc_a * 4000);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, dc_b * 4000);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_3, dc_c * 4000);
}

void setPhaseVoltage(float Uq, float Ud, float angle_el)
{
    angle_el = _normalizeAngle(angle_el + zero_electric_angle);
    // 帕克逆变换
    Ualpha = -Uq * sin(angle_el);
    Ubeta = Uq * cos(angle_el);

    // 克拉克逆变换
    Ua = Ualpha + voltage_power_supply / 2;
    Ub = (sqrt(3) * Ubeta - Ualpha) / 2 + voltage_power_supply / 2;
    Uc = (-Ualpha - sqrt(3) * Ubeta) / 2 + voltage_power_supply / 2;
    setPwm(Ua, Ub, Uc);
}

// 开环速度函数
float velocityOpenloop(float target_velocity)
{
    unsigned long now_us = micros(); // 获取从开启芯片以来的微秒数，它的精度是 4 微秒。 micros() 返回的是一个无符号长整型（unsigned long）的值

    // 计算当前每个Loop的运行时间间隔
    float Ts = (now_us - open_loop_timestamp) * 1e-6f;

    // 由于 micros() 函数返回的时间戳会在大约 70 分钟之后重新开始计数，在由70分钟跳变到0时，TS会出现异常，因此需要进行修正。如果时间间隔小于等于零或大于 0.5 秒，则将其设置为一个较小的默认值，即 1e-3f
    if (Ts <= 0 || Ts > 0.5f)
        Ts = 1e-3f;

    // 通过乘以时间间隔和目标速度来计算需要转动的机械角度，存储在 shaft_angle 变量中。在此之前，还需要对轴角度进行归一化，以确保其值在 0 到 2π 之间。
    shaft_angle = _normalizeAngle(shaft_angle + target_velocity * Ts);
    // 以目标速度为 10 rad/s 为例，如果时间间隔是 1 秒，则在每个循环中需要增加 10 * 1 = 10 弧度的角度变化量，才能使电机转动到目标速度。
    // 如果时间间隔是 0.1 秒，那么在每个循环中需要增加的角度变化量就是 10 * 0.1 = 1 弧度，才能实现相同的目标速度。因此，电机轴的转动角度取决于目标速度和时间间隔的乘积。

    // 使用早前设置的voltage_power_supply的1/3作为Uq值，这个值会直接影响输出力矩
    // 最大只能设置为Uq = voltage_power_supply/2，否则ua,ub,uc会超出供电电压限幅
    float Uq = voltage_power_supply / 6; // 3;

    setPhaseVoltage(Uq, 0, _electricalAngle(shaft_angle, 7));

    open_loop_timestamp = now_us; // 用于计算下一个时间间隔

    return Uq;
}