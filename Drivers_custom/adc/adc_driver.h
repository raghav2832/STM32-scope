#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/*
 * ADC1, Channel 0, PA0
 * Single-shot, software trigger, 12-bit right-aligned
 *
 * Sampling time options (SMP bits, 3 bits per channel):
 *   0 →   3 cycles
 *   1 →  15 cycles
 *   2 →  28 cycles
 *   3 →  56 cycles
 *   4 →  84 cycles
 *   5 → 112 cycles
 *   6 → 144 cycles
 *   7 → 480 cycles   ← use for high-impedance sources
 */

#define ADC_SAMPLETIME_3    0U
#define ADC_SAMPLETIME_15   1U
#define ADC_SAMPLETIME_28   2U
#define ADC_SAMPLETIME_56   3U
#define ADC_SAMPLETIME_84   4U
#define ADC_SAMPLETIME_112  5U
#define ADC_SAMPLETIME_144  6U
#define ADC_SAMPLETIME_480  7U

void     adc_init(uint8_t sample_time);
uint16_t adc_read_single(uint8_t channel);
float    adc_to_voltage(uint16_t raw, float vref);

/* Add this to existing adc_driver.h */
void adc_init_dma(uint8_t sample_time);
void adc_start_dma(void);
void adc_stop_dma(void);

#endif
