/* Core/Src/stm32f4xx_it.c */

#include "stm32f4xx.h"

/* ----------------------------------------------------------------
   Default fault handlers — these catch crashes and hang the MCU
   so you can attach a debugger and see what went wrong.
   ---------------------------------------------------------------- */

void NMI_Handler(void)
{
    while (1);
}

void HardFault_Handler(void)
{
    /* Put a breakpoint here in debugger to catch faults */
    while (1);
}

void MemManage_Handler(void)
{
    while (1);
}

void BusFault_Handler(void)
{
    while (1);
}

void UsageFault_Handler(void)
{
    while (1);
}

/* ----------------------------------------------------------------
   SysTick — called every 1ms once we configure it
   We call our own callback, defined in Util/systick.c
   ---------------------------------------------------------------- */
extern void systick_callback(void);

void SysTick_Handler(void)
{
    systick_callback();
}
