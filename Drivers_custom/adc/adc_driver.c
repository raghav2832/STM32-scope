#include "adc_driver.h"
#include "gpio_driver.h"

/*
 * ADC Register Map — key registers we touch:
 *
 *  ADC->SR    status register    (EOC flag lives here)
 *  ADC->CR1   control 1          (resolution, scan mode)
 *  ADC->CR2   control 2          (enable, software trigger, alignment)
 *  ADC->SMPR1 sample time ch10+  (channels 10-18)
 *  ADC->SMPR2 sample time ch0-9  (channels 0-9)
 *  ADC->SQR1  sequence length    (how many conversions)
 *  ADC->SQR3  sequence order     (which channel first)
 *  ADC->DR    data register      (result lives here)
 *
 *  ADC_Common->CCR  common control (prescaler, temp sensor, vref)
 */

void adc_init(uint8_t sample_time)
{
    /* 1. Enable ADC1 clock (APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    volatile uint32_t dummy = RCC->APB2ENR;
    (void)dummy;

    /* 2. Configure PA0 as analog input
     *    Analog mode = no pull, no output driver, no AF
     *    This disconnects the digital input buffer → reduces noise
     */
    GPIO_PinCfg_t pa0 = {
        .port  = GPIOA,
        .pin   = 0,
        .mode  = GPIO_MODE_ANALOG,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_LOW,
        .pupd  = GPIO_PUPD_NONE,
        .af    = 0
    };
    gpio_init(&pa0);

    /*
     * 3. ADC Common: set prescaler
     *    CCR ADCPRE bits:
     *      00 → /2   (45MHz — too fast, exceeds 36MHz max)
     *      01 → /4   (22.5MHz — OK)
     *      10 → /6   (15MHz)
     *      11 → /8   (11.25MHz)
     *    We use /4 → 22.5MHz ADC clock
     */
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  (1U << ADC_CCR_ADCPRE_Pos);   /* /4 */

    /* 4. CR1: 12-bit resolution, no scan mode */
    ADC1->CR1 = 0;
    /*
     * RES bits in CR1[25:24]:
     *   00 → 12-bit
     *   01 → 10-bit
     *   10 →  8-bit
     *   11 →  6-bit
     * Default 0 = 12-bit, fine.
     */

    /* 5. CR2: right alignment, software trigger, no continuous */
    ADC1->CR2 = 0;
    /*
     * ALIGN bit = 0 → right aligned (raw value 0–4095)
     * CONT  bit = 0 → single conversion mode
     * EXTEN bits = 00 → software trigger only
     */

    /*
     * 6. Sample time for channel 0
     *    Channel 0 is in SMPR2 (channels 0-9)
     *    3 bits per channel, channel 0 is at bits [2:0]
     */
    ADC1->SMPR2 &= ~(7U << (3U * 0U));
    ADC1->SMPR2 |=  (sample_time << (3U * 0U));

    /* 7. Sequence: 1 conversion, channel 0 first */
    ADC1->SQR1  = 0;   /* L bits = 0000 → 1 conversion */
    ADC1->SQR3  = 0;   /* SQ1 = channel 0 */

    /* 8. Enable ADC */
    ADC1->CR2 |= ADC_CR2_ADON;

    /*
     * 9. Wait for ADC to stabilize
     *    After ADON, ADC needs a stabilization time (t_stab).
     *    Datasheet says max 3µs. We delay 10µs to be safe.
     *    We do not have a us delay yet — a simple loop is fine here
     *    since this is one-time init.
     */
    for (volatile uint32_t i = 0; i < 1800U; i++);
    /* 1800 iterations at 180MHz ≈ 10µs */
}

uint16_t adc_read_single(uint8_t channel)
{
    /*
     * For single-shot: update the sequence register with the
     * requested channel, then trigger a conversion.
     * This lets us read any channel without reinitializing.
     */

    /* Set channel in SQR3 (first conversion in sequence) */
    ADC1->SQR3 = (channel & 0x1FU);

    /* Update sample time for this channel
     * Channels 0-9 → SMPR2, channels 10-18 → SMPR1
     * We read current sample time from channel 0 setting and apply it
     */
    uint8_t smp = (ADC1->SMPR2 >> (3U * 0U)) & 0x7U;
    if (channel <= 9U) {
        ADC1->SMPR2 &= ~(7U << (3U * channel));
        ADC1->SMPR2 |=  (smp << (3U * channel));
    } else {
        uint8_t ch = channel - 10U;
        ADC1->SMPR1 &= ~(7U << (3U * ch));
        ADC1->SMPR1 |=  (smp << (3U * ch));
    }

    /* Clear EOC flag */
    ADC1->SR &= ~ADC_SR_EOC;

    /* Software start conversion */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* Wait for End Of Conversion */
    while (!(ADC1->SR & ADC_SR_EOC));

    /* Read result — this also clears EOC */
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

float adc_to_voltage(uint16_t raw, float vref)
{
    /*
     * 12-bit ADC: full scale = 4095 counts = vref volts
     * voltage = raw * vref / 4095
     * vref on Nucleo = 3.3V (VDDA pin)
     */
    return ((float)raw * vref) / 4095.0f;
}
void adc_init_dma(uint8_t sample_time)
{
    /* 1. Enable ADC1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    volatile uint32_t dummy = RCC->APB2ENR;
    (void)dummy;

    /* 2. PA0 → analog */
    GPIO_PinCfg_t pa0 = {
        .port  = GPIOA,
        .pin   = 0,
        .mode  = GPIO_MODE_ANALOG,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_LOW,
        .pupd  = GPIO_PUPD_NONE,
        .af    = 0
    };
    gpio_init(&pa0);

    /* 3. ADC prescaler /4 → 22.5MHz */
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  (1U << ADC_CCR_ADCPRE_Pos);

    /* 4. CR1: 12-bit, no scan */
    ADC1->CR1 = 0;

    /*
     * 5. CR2: timer trigger
     *
     * EXTEN bits [29:28]: external trigger enable
     *   00 = disabled (software)
     *   01 = rising edge   ← we use this
     *   10 = falling edge
     *   11 = both edges
     *
     * EXTSEL bits [27:24]: trigger source
     *   For ADC1, TIM2_TRGO = 1011 (0xB)
     *   Check RM0390 Table 99 for full list
     *
     * DDS bit [9]: DMA disable selection
     *   0 = DMA requests stop after last transfer (single mode)
     *   1 = DMA requests continue (circular)   ← we need this
     *
     * DMA bit [8]: enable DMA requests from ADC
     */
    ADC1->CR2 = (1U  << ADC_CR2_EXTEN_Pos)    /* rising edge trigger */
              | (6U << ADC_CR2_EXTSEL_Pos)   /* TIM2_TRGO */
              | ADC_CR2_DDS                    /* DMA circular */
              | ADC_CR2_DMA;                   /* enable DMA */

    /* 6. Sample time for channel 0 */
    ADC1->SMPR2 &= ~(7U << (3U * 0U));
    ADC1->SMPR2 |=  (sample_time << (3U * 0U));

    /* 7. Sequence: 1 conversion, channel 0 */
    ADC1->SQR1 = 0;
    ADC1->SQR3 = 0;

    /* 8. Enable ADC */
    ADC1->CR2 |= ADC_CR2_ADON;

    /* Stabilization delay */
    for (volatile uint32_t i = 0; i < 1800U; i++);
}

void adc_start_dma(void)
{
    /* ADC is already on. Trigger comes from TIM2.
     * Just make sure DMA is started before TIM2 starts.
     * TIM2 start is called from application code after this. */
}

void adc_stop_dma(void)
{
    /* Stop external trigger by clearing EXTEN */
    ADC1->CR2 &= ~ADC_CR2_EXTEN;
}
