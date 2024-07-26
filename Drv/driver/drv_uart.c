#include "drv_uart.h"

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#define RECV_SIZE 64

lwrb_t rx_ringbuf;
lwrb_t tx_ringbuf;

/**
 * @brief   Ring buffer data array for RX DMA
 */
uint8_t rx_ringbuf_array[256];

/**
 * @brief   Ring buffer data array for TX DMA
 * @note    would printf error if use local variable
 */
uint8_t tx_ringbuf_array[512];

/**
 * @brief   uart dma recieve buffer
 */
uint8_t recv_buf[RECV_SIZE];
uint16_t recv_len;

/**
 * @brief           Length of currently active TX DMA transfer
 */
volatile size_t usart_tx_dma_current_len;

static void dma_recv_HT(struct __DMA_HandleTypeDef *hdma);
static void dma_recv_TC(struct __DMA_HandleTypeDef *hdma);

void uart_init(UART_HandleTypeDef *huart)
{
    /* dma RX and TX ringbuffer initialization */
    lwrb_init(&tx_ringbuf, tx_ringbuf_array, sizeof(tx_ringbuf_array));
    lwrb_init(&rx_ringbuf, rx_ringbuf_array, sizeof(rx_ringbuf_array));

    /* dma receive config */
    HAL_UART_Receive_DMA(huart, recv_buf, RECV_SIZE);
    __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_HT);
    __HAL_DMA_ENABLE_IT(huart->hdmarx, DMA_IT_TC);
    HAL_DMA_RegisterCallback(huart->hdmarx, HAL_DMA_XFER_HALFCPLT_CB_ID, dma_recv_HT);
    HAL_DMA_RegisterCallback(huart->hdmarx, HAL_DMA_XFER_CPLT_CB_ID, dma_recv_TC);

    /* usart config */
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
}

void uart_send(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        while (__HAL_UART_GET_FLAG(huart, UART_FLAG_TXE) == RESET)
            ;

        huart->Instance->TDR = pData[i];

        while (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET)
            ;
    }
}
void uart_send_dma(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t len)
{
    lwrb_write(&tx_ringbuf, pData, len); /* Write data to TX buffer for loopback */
    uart_dma_transfer(huart);
}

void uart_dma_transfer(UART_HandleTypeDef *huart)
{
    if (usart_tx_dma_current_len == 0)
    {
        usart_tx_dma_current_len = lwrb_get_linear_block_read_length(&tx_ringbuf);
        if (usart_tx_dma_current_len > 0)
            HAL_UART_Transmit_DMA(huart, (uint8_t *)lwrb_get_linear_block_read_address(&tx_ringbuf), usart_tx_dma_current_len);
    }
}

void uart_printf(UART_HandleTypeDef *huart, const char *fmt, ...)
{
    char buf[128];
    uint16_t len;
    va_list arg;

    va_start(arg, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);

    uart_send_dma(huart, (uint8_t *)buf, len);
}

static void uart_rx_check(DMA_HandleTypeDef *hdma)
{
    static size_t old_pos;
    size_t pos;

    /* Calculate current position in buffer and check for new data available */
    pos = ARRAY_LEN(recv_buf) - __HAL_DMA_GET_COUNTER(hdma);
    if (pos != old_pos)
    {
        /* Check change in received data */
        if (pos > old_pos)
        {
            /* Current position is over previous one */
            lwrb_write(&rx_ringbuf, &recv_buf[old_pos], pos - old_pos);
        }
        else
        {
            lwrb_write(&rx_ringbuf, &recv_buf[old_pos], ARRAY_LEN(recv_buf) - old_pos);
            if (pos > 0)
            {
                lwrb_write(&rx_ringbuf, &recv_buf[0], pos);
            }
        }
        old_pos = pos; /* Save current position as old for next transfers */
    }
}

static void dma_recv_HT(struct __DMA_HandleTypeDef *hdma)
{
    uart_rx_check(hdma);
}

static void dma_recv_TC(struct __DMA_HandleTypeDef *hdma)
{
    uart_rx_check(hdma);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    lwrb_skip(&tx_ringbuf, usart_tx_dma_current_len); /* Skip buffer, it has been successfully sent out */
    usart_tx_dma_current_len = 0;                     /* Reset data length */
    uart_dma_transfer(huart);
}

void HAL_UARTEx_IdleCallback(UART_HandleTypeDef *huart)
{
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        uart_rx_check(huart->hdmarx);
    }
}
