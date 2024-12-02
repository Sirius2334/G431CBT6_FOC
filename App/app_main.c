#include "app_main.h"

#define JScopeBufferSize 4096

#define TS_CAL1 *((__IO uint16_t *)0x1FFF75A8) // 单位为mV
#define TS_CAL2 *((__IO uint16_t *)0x1FFF75CA) // 单位为mV

#define TS_TEMP_CAL1 30  // 温度为30度时，其AD值为TS_CAL1
#define TS_TEMP_CAL2 130 // 温度为130度时，其AD值为TS_CAL2

#define CalculateInternalTemperature(TS_DATA) (float)(TS_TEMP_CAL2 - TS_TEMP_CAL1) / (TS_CAL2 - TS_CAL1) * ((TS_DATA * 1.1) - TS_CAL1) + TS_TEMP_CAL1

uint8_t *JScopeBuffer[JScopeBufferSize];
uint32_t uAdcValue, vAdcValue, wAdcValue;
float uVoltage, vVoltage, wVoltage;

void adc_get_value(void)
{
    // HAL_ADCEx_InjectedStart(&hadc1);
    // HAL_ADCEx_InjectedStart(&hadc2);
    uAdcValue = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
    vAdcValue = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
    wAdcValue = HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);

    SEGGER_RTT_Write(1, &uAdcValue, 2);
    SEGGER_RTT_Write(1, &vAdcValue, 2);
    SEGGER_RTT_Write(1, &wAdcValue, 2);
}

void app_main(void)
{
    char c;
    // uint16_t TS_CAL1, TS_CAL2;
    uint32_t adc_temp;
    float temperture;

    HAL_OPAMP_Start(&hopamp1);
    HAL_OPAMP_Start(&hopamp2);
    HAL_OPAMP_Start(&hopamp3);
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_3, 0);

    HAL_ADCEx_InjectedStart(&hadc1);
    HAL_ADCEx_InjectedStart(&hadc2);
    __HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_JEOC);
    __HAL_ADC_ENABLE_IT(&hadc2, ADC_IT_JEOC);

    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, 3950);

    uart_init(&huart1);
    userShellInit(&shell);

    rtt_init();
    SEGGER_RTT_ConfigUpBuffer(1, "JScope_u2u2u2", JScopeBuffer, JScopeBufferSize, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

    rtt_printf("Hello World!\r\n");
    // uart_printf(&huart1, "hello world, arg = %.2f\r\n", 3.14);

    // TS_CAL1 = *(__IO uint16_t *)(0x1FFF75A8);
    // TS_CAL2 = *(__IO uint16_t *)(0x1FFF75CA);

    float angle = 0;

    for (;;)
    {
        if (lwrb_read(&rx_ringbuf, &c, 1))
        {
            shellHandler(&shell, c);
        }
        HAL_Delay(10);

        angle = MT6701_GetFullAngle();
        uart_printf(&huart1, "angle = %.2f\r\n", angle);
        HAL_Delay(1000);

        HAL_ADC_Start(&hadc1);
        adc_temp = HAL_ADC_GetValue(&hadc1); /* 读取数值 */
                                             // temperture = (float)(110 - 30) / (TS_CAL2 - TS_CAL1) * (adc_temp - TS_CAL1) + 30; /* 转换 */
        temperture = __HAL_ADC_CALC_TEMPERATURE(3300, adc_temp, ADC_RESOLUTION_12B);
        temperture = CalculateInternalTemperature(adc_temp);

        uart_printf(&huart1, "chip temperture = %.2f\r\n", temperture);

        // uVoltage = 3.3f * uAdcValue / 4096;
        // vVoltage = 3.3f * vAdcValue / 4096;
        // wVoltage = 3.3f * wAdcValue / 4096;

        // // uart_printf(&huart1, "adc value = %d, %d, %d\r\n", uAdcValue, vAdcValue, wAdcValue);
        // uart_printf(&huart1, "adc value = %f, %f, %f\r\n", uVoltage, vVoltage, wVoltage);
        // HAL_Delay(1000);
    }
}

int func(int argc, char *argv[])
{
    rtt_printf("%dparameter(s)\r\n", argc);
    uart_printf(&huart1, "%dparameter(s)\r\n", argc);
    for (uint8_t i = 1; i < argc; i++)
    {
        rtt_printf("%s\r\n", (char *)argv[i]);
        uart_printf(&huart1, "%s\r\n", (char *)argv[i]);
    }

    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
                 func,
                 func,
                 test);

int soft_reset(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    HAL_NVIC_SystemReset();
    return 0;
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
                 reset,
                 soft_reset,
                 soft reset);
