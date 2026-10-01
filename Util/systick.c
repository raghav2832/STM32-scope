#include "systick.h"
#include "stm32f4xx.h"

static volatile uint32_t g_tick = 0;

void systick_init(uint32_t sys_clk_hz)
{
    SysTick->LOAD = (sys_clk_hz / 1000U) - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk   /* processor clock */
                  | SysTick_CTRL_TICKINT_Msk      /* enable interrupt */
                  | SysTick_CTRL_ENABLE_Msk;      /* start counter */
}

void systick_callback(void)
{
    g_tick++;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = g_tick;
    while ((g_tick - start) < ms);
}

uint32_t millis(void)
{
    return g_tick;
}
