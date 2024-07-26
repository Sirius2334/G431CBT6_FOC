#include "shell.h"
#include "shell_port.h"

#define SHELL_BUFF_SIZE 512

Shell shell;
char shellBuffer[SHELL_BUFF_SIZE];

/**
 * @brief 用户shell写
 *
 * @param data 数据
 * @param len 数据长度
 *
 * @return short 实际写入的数据长度
 */
short userShellWrite(char *data, unsigned short len)
{
    uart_send_dma(&huart1, (uint8_t *)data, len);
    return len;
}

/**
 * @brief 用户shell初始化
 *
 */
void userShellInit(Shell *shell)
{
    shell->write = userShellWrite;

    shellInit(shell, shellBuffer, SHELL_BUFF_SIZE);
}
