#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void     systick_init(uint32_t sys_clk_hz);
void     systick_callback(void);       /* called from stm32f4xx_it.c */
void     delay_ms(uint32_t ms);
uint32_t millis(void);

#endif
