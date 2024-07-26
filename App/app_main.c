#include "app_main.h"

void app_main(void)
{
    char string[10], c;
    uint32_t rb_len;
    int16_t temp;

    rtt_init();
    uart_init(&huart1);
    userShellInit(&shell);

    rtt_printf("Hello World!\r\n");
    uart_printf(&huart1, "hello world, arg = %.2f\r\n", 3.14);

    for (;;)
    {
        // rb_len = (uint32_t)lwrb_read(&rx_ringbuf, string, 9);
        // if (rb_len != 0)
        // {
        //     uart_printf(&huart1, "ringbuff return string = \"%s\"\r\n", string);
        //     // HAL_Delay(5);
        //     uart_printf(&huart1, "ringbuff return len = %d\r\n", rb_len);
        // }

        if (lwrb_read(&rx_ringbuf, &c, 1))
        {
            shellHandler(&shell, c);
        }
        HAL_Delay(10);

        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
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
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), func, func, test);
