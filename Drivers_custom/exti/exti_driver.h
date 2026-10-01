#ifndef EXTI_DRIVER_H
#define EXTI_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/*
 * EXTI lines map to GPIO pins by number:
 *   EXTI0 → Px0 (any port, pin 0)
 *   EXTI1 → Px1 (any port, pin 1)
 *   etc.
 *
 * Only one port can use a given EXTI line at a time.
 * SYSCFG_EXTICRx selects which port drives each line.
 *
 * Port encoding for SYSCFG:
 *   PA=0, PB=1, PC=2, PD=3, PE=4
 */

#define EXTI_PORT_A   0U
#define EXTI_PORT_B   1U
#define EXTI_PORT_C   2U
#define EXTI_PORT_D   3U

#define EXTI_EDGE_RISING   0U
#define EXTI_EDGE_FALLING  1U
#define EXTI_EDGE_BOTH     2U

typedef void (*exti_callback_t)(uint8_t pin, uint8_t level);

void exti_init(uint8_t pin,
               uint8_t port_sel,
               uint8_t edge,
               exti_callback_t cb);

#endif
