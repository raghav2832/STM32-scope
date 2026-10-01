#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/*
 * ADC1 uses DMA2, Stream 0, Channel 0
 * (from STM32F4 reference manual, Table 42 DMA2 request mapping)
 *
 * Stream 0 = DMA2_Stream0
 * Channel 0 = selected via CHSEL bits in SxCR
 */

#define ADC_DMA_STREAM     DMA2_Stream0
#define ADC_DMA_CHANNEL    0U

/*
 * Callback function types.
 * The ISR calls these when half or full transfer completes.
 * Defined in application code (scope.c or main.c for now).
 */
typedef void (*dma_callback_t)(void);

void dma_adc_init(uint16_t       *buffer,
                  uint16_t        length,
                  dma_callback_t  half_cb,
                  dma_callback_t  full_cb);

void dma_adc_start(void);
void dma_adc_stop(void);

#endif
