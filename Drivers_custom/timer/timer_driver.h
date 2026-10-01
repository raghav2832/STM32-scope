#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/*
 * Clock facts for F446RE at 180MHz SYSCLK:
 *
 *   APB2 = 90MHz  →  TIM1 clock = 90 × 2 = 180MHz
 *   APB1 = 45MHz  →  TIM2 clock = 45 × 2 = 90MHz
 *
 * (When APB prescaler != 1, timer clock = 2 × APB clock)
 *
 * PWM frequency:
 *   f = TIM_CLK / ((PSC+1) × (ARR+1))
 *
 * Our strategy: always set PSC so tick = 1MHz
 *   PSC = TIM_CLK / 1,000,000 - 1
 *   ARR = 1,000,000 / freq_hz  - 1
 *   CCR = (ARR+1) * duty / 100
 */

#define TIM1_CLK_HZ    180000000U   /* APB2(90MHz) × 2 */
#define TIM2_CLK_HZ     90000000U   /* APB1(45MHz) × 2 */

void tim1_pwm_init    (uint32_t freq_hz, uint8_t duty_pct);
void tim1_pwm_set_duty(uint8_t duty_pct);
void tim1_pwm_set_freq(uint32_t freq_hz);

/* Add to existing timer_driver.h */
void tim2_init_trgo(uint32_t sample_rate_hz);
void tim2_start(void);
void tim2_stop(void);

#endif
