#include "drv_mt6701.h"

#include <math.h>
#include <stdint.h>

#include "drv_uart.h"

#define _2PI 6.28318531f
#define _3PI_2 4.71238898f

#define MT6701_CS_Enable() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET)
#define MT6701_CS_Disable() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET)

const uint8_t pole_pairs = 7; // 极对数
float zeroElectricAngleOffset = 0.0f;

int rotationCount = 0;  // 旋转过的圈数
int rotationCount_Last; // 上一次循环时转过的圈数

// 获取MT6701原始数据
uint16_t MT6701_GetRawData(void)
{
    uint16_t rawData;
    uint16_t timeOut = 200;

    while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY)
    {
        if (timeOut-- == 0)
        {
            uart_printf(&huart1, "SPI state error!\r\n");
            return 0; // 在超时时直接返回，避免继续执行后续代码
        }
    }

    MT6701_CS_Enable();

    HAL_StatusTypeDef spiStatus = HAL_SPI_Receive(&hspi1, (uint8_t *)&rawData, 1, HAL_MAX_DELAY);
    // HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)&txData, (uint8_t *)&rawData, 1, HAL_MAX_DELAY);
    if (spiStatus != HAL_OK)
    {
        MT6701_CS_Disable();
        uart_printf(&huart1, "MT6701 read data error!\r\n");
        return 0; // 在SPI传输错误时直接返回，避免继续执行后续代码
    }

    MT6701_CS_Disable();

    return rawData >> 2; // 取高14位的角度数据
}

// 获得原始角度，无圈数累加
float MT6701_GetRawAngle(void)
{
    uint16_t rawData = MT6701_GetRawData();
    return (float)rawData / 16384.0f * _2PI;
}

// 获得转过的总角度，有圈数累加
float MT6701_GetFullAngle(void)
{
    static float angle_Last = 0.0f;     // 上次的轴角度，范围0~6.28
    float angle = MT6701_GetRawAngle(); // 当前角度，范围0~6.28
    float deltaAngle = angle - angle_Last;

    // 计算旋转的总圈数
    // 通过判断角度变化是否大于80%的一圈(0.8f*6.28318530718f)来判断是否发生了溢出，如果发生了，则将full_rotations增加1（如果d_angle小于0）或减少1（如果d_angle大于0）。

    if (fabsf(deltaAngle) > (0.8f * 6.28318530718f))
    {
        rotationCount += (deltaAngle > 0) ? -1 : 1; // 圈数计算
        rotationCount_Last = rotationCount;
    }

    angle_Last = angle;
    return rotationCount * 6.28318530718f + angle_Last; // 转过的圈数 * 2pi + 未转满一圈的角度值
}

// 计算转速
float MT6701_GetVelocity(void)
{
    static float full_Angle_Last = 0.0f; // 记录上次转过的总角度
    float full_Angle = MT6701_GetFullAngle();
    float delta_Angle = (rotationCount - rotationCount_Last) * _2PI + (full_Angle - full_Angle_Last);
    float vel = delta_Angle * 1000.0f; // Ts = 1ms

    // 更新变量值
    full_Angle_Last = full_Angle;
    return vel;
}

// 获得电角度
float MT6701_GetElectricalAngle(void)
{
    return _normalizeAngle(pole_pairs * MT6701_GetRawAngle() - zeroElectricAngleOffset);
}

// 电角度零位校准
void Align_Sensor(void)
{
    setPhaseVoltage(3.0f, 0.0f, _3PI_2);
    HAL_Delay(2000);
    zeroElectricAngleOffset = MT6701_GetElectricalAngle();
    setPhaseVoltage(0, 0, _3PI_2);
    uart_printf(&huart1, "[zeroAngle]:%f\r\n", zeroElectricAngleOffset);
}