#include "timer_driver.h"
#include "gpio_driver.h"

void tim1_pwm_init(uint32_t freq_hz, uint8_t duty_pct)
{
    /* 1. Enable TIM1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    volatile uint32_t dummy = RCC->APB2ENR;
    (void)dummy;

    /* 2. PA8 → AF1 (TIM1_CH1) */
    GPIO_PinCfg_t pa8 = {
        .port  = GPIOA,
        .pin   = 8,
        .mode  = GPIO_MODE_AF,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_HIGH,
        .pupd  = GPIO_PUPD_NONE,
        .af    = 1U
    };
    gpio_init(&pa8);

    /* 3. Reset timer registers */
    TIM1->CR1   = 0;
    TIM1->CR2   = 0;
    TIM1->SMCR  = 0;
    TIM1->CCMR1 = 0;
    TIM1->CCER  = 0;
    TIM1->BDTR  = 0;

    /* 4. PSC and ARR for target frequency */
    uint32_t psc = (TIM1_CLK_HZ / 1000000U) - 1U;  /* tick = 1MHz */
    uint32_t arr = (1000000U / freq_hz) - 1U;

    TIM1->PSC = psc;
    TIM1->ARR = arr;

    /* 5. PWM mode 1 on CH1, preload enable */
    TIM1->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;

    /* 6. Duty cycle */
    TIM1->CCR1 = ((arr + 1U) * duty_pct) / 100U;

    /* 7. Enable CH1 output */
    TIM1->CCER |= TIM_CCER_CC1E;

    /* 8. Auto-reload preload */
    TIM1->CR1 |= TIM_CR1_ARPE;

    /*
     * 9. MOE — Main Output Enable
     * TIM1 is an advanced timer. This bit MUST be set
     * or nothing appears on PA8 regardless of other config.
     */
    TIM1->BDTR |= TIM_BDTR_MOE;

    /* 10. Force update to load shadow registers */
    TIM1->EGR |= TIM_EGR_UG;

    /* 11. Start */
    TIM1->CR1 |= TIM_CR1_CEN;
}

void tim1_pwm_set_duty(uint8_t duty_pct)
{
    uint32_t arr = TIM1->ARR;
    TIM1->CCR1 = ((arr + 1U) * duty_pct) / 100U;
}

void tim1_pwm_set_freq(uint32_t freq_hz)
{
    TIM1->ARR  = (1000000U / freq_hz) - 1U;
    TIM1->EGR |= TIM_EGR_UG;
}
void tim2_init_trgo(uint32_t sample_rate_hz)
{
    /* Enable TIM2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    volatile uint32_t dummy = RCC->APB1ENR;
    (void)dummy;

    TIM2->CR1  = 0;
    TIM2->CR2  = 0;
    TIM2->SMCR = 0;

    /*
     * TIM2 clock = APB1 × 2 = 90MHz
     * PSC = 89  → tick = 90MHz/90 = 1MHz
     * ARR = (1MHz / sample_rate) - 1
     *
     * At 100KSPS: ARR = 1000000/100000 - 1 = 9
     */
    TIM2->PSC = (TIM2_CLK_HZ / 1000000U) - 1U;
    TIM2->ARR = (1000000U / sample_rate_hz) - 1U;

    /*
     * CR2 MMS bits [6:4]: master mode selection
     * MMS = 010 → Update event as TRGO
     * This fires TRGO every time TIM2 overflows
     * ADC is configured to start on this trigger
     */
    TIM2->CR2 |= (2U << TIM_CR2_MMS_Pos);

    /* Load registers immediately */
    TIM2->EGR |= TIM_EGR_UG;

    /* Clear update flag set by UG */
    TIM2->SR &= ~TIM_SR_UIF;
}

void tim2_start(void)
{
    TIM2->CR1 |= TIM_CR1_CEN;
}

void tim2_stop(void)
{
    TIM2->CR1 &= ~TIM_CR1_CEN;
}
