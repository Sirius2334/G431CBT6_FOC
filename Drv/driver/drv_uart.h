#ifndef __DRV_UART_H_
#define __DRV_UART_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "usart.h"
#include "lwrb.h"

extern lwrb_t rx_ringbuf;
extern lwrb_t tx_ringbuf;
extern uint8_t rx_ringbuf_array[256];
extern uint8_t tx_ringbuf_array[512];

void uart_init(UART_HandleTypeDef *huart);
void uart_send(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t len);
void uart_send_dma(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t len);
void uart_printf(UART_HandleTypeDef *huart, const char *fmt, ...);
void uart_dma_transfer(UART_HandleTypeDef *huart);
void HAL_UARTEx_IdleCallback(UART_HandleTypeDef *huart);

#endif