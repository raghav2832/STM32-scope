#include "uart_driver.h"
#include "stm32f4xx.h"

void uart_init(uint32_t baud, uint32_t apb1_clk_hz)
{
    /* 1. Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* Small delay after enabling clock before accessing registers */
    volatile uint32_t dummy = RCC->APB1ENR;
    (void)dummy;

    /* 2. PA2=TX, PA3=RX → Alternate Function 7 */
    /* Mode: AF (10) */
    GPIOA->MODER &= ~((3U << (2*2)) | (3U << (3*2)));
    GPIOA->MODER |=  ((2U << (2*2)) | (2U << (3*2)));

    /* AF7 for PA2 and PA3 (AFR[0] covers pins 0-7) */
    GPIOA->AFR[0] &= ~((0xFU << (4*2)) | (0xFU << (4*3)));
    GPIOA->AFR[0] |=  ((7U   << (4*2)) | (7U   << (4*3)));

    /* Output speed: high */
    GPIOA->OSPEEDR |= (3U << (2*2)) | (3U << (3*2));

    /* Pull-up on RX */
    GPIOA->PUPDR &= ~(3U << (3*2));
    GPIOA->PUPDR |=  (1U << (3*2));

    /* 3. USART2 config */
    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    /* BRR = fPCLK / baud
       45000000 / 115200 = 390.625
       Mantissa = 390, Fraction = 0.625 * 16 = 10
       BRR = (390 << 4) | 10 = 6250 */
    USART2->BRR = (apb1_clk_hz + (baud / 2U)) / baud;

    /* Enable TX + RX + USART */
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    /* Wait until TX is ready */
    while (!(USART2->SR & USART_SR_TC));
}

void uart_send_byte(uint8_t byte)
{
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = byte;
}

void uart_send_buf(const uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        uart_send_byte(buf[i]);
}

uint8_t uart_recv_byte(void)
{
    while (!(USART2->SR & USART_SR_RXNE));
    return (uint8_t)USART2->DR;
}

uint8_t uart_data_ready(void)
{
    return (USART2->SR & USART_SR_RXNE) ? 1U : 0U;
}

/* printf redirect — newlib calls _write for stdout */
int _write(int file, char *ptr, int len)
{
    (void)file;
    uart_send_buf((uint8_t*)ptr, (uint16_t)len);
    return len;
}
