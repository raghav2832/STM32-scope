#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>

/*
 * USART2: PA2=TX, PA3=RX
 * This is the ST-Link virtual COM port on Nucleo.
 * No external hardware needed.
 */

void    uart_init(uint32_t baud, uint32_t apb1_clk_hz);
void    uart_send_byte(uint8_t byte);
void    uart_send_buf(const uint8_t *buf, uint16_t len);
uint8_t uart_recv_byte(void);
uint8_t uart_data_ready(void);

#endif
