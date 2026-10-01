#include "dma_driver.h"

/* Callbacks stored at init time, called from ISR */
static dma_callback_t s_half_cb = 0;
static dma_callback_t s_full_cb = 0;

void dma_adc_init(uint16_t      *buffer,
                  uint16_t       length,
                  dma_callback_t half_cb,
                  dma_callback_t full_cb)
{
    s_half_cb = half_cb;
    s_full_cb = full_cb;

    /* 1. Enable DMA2 clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    volatile uint32_t dummy = RCC->AHB1ENR;
    (void)dummy;

    /* 2. Disable stream before configuring */
    ADC_DMA_STREAM->CR &= ~DMA_SxCR_EN;
    while (ADC_DMA_STREAM->CR & DMA_SxCR_EN);

    /* 3. Clear all interrupt flags for Stream 0
     *    Stream 0 flags are in LIFCR bits [5:0]
     *    FEIF0=bit0, DMEIF0=bit2, TEIF0=bit3, HTIF0=bit4, TCIF0=bit5
     */
    DMA2->LIFCR = DMA_LIFCR_CFEIF0
                | DMA_LIFCR_CDMEIF0
                | DMA_LIFCR_CTEIF0
                | DMA_LIFCR_CHTIF0
                | DMA_LIFCR_CTCIF0;

    /*
     * 4. Configure stream
     *
     * CHSEL  = 0       → channel 0 (ADC1)
     * DIR    = 00      → peripheral to memory
     * CIRC   = 1       → circular mode (auto-restarts)
     * MINC   = 1       → memory address increments
     * PINC   = 0       → peripheral address fixed (ADC DR)
     * MSIZE  = 01      → memory data size = 16-bit
     * PSIZE  = 01      → peripheral data size = 16-bit
     * PL     = 10      → priority high
     * HTIE   = 1       → half transfer interrupt enable
     * TCIE   = 1       → transfer complete interrupt enable
     * TEIE   = 1       → transfer error interrupt enable
     */
    ADC_DMA_STREAM->CR = (ADC_DMA_CHANNEL << DMA_SxCR_CHSEL_Pos)
                       | (0U  << DMA_SxCR_DIR_Pos)
                       | DMA_SxCR_CIRC
                       | DMA_SxCR_MINC
                       | (1U  << DMA_SxCR_MSIZE_Pos)
                       | (1U  << DMA_SxCR_PSIZE_Pos)
                       | (2U  << DMA_SxCR_PL_Pos)
                       | DMA_SxCR_HTIE
                       | DMA_SxCR_TCIE
                       | DMA_SxCR_TEIE;

    /* 5. Peripheral address = ADC1 data register */
    ADC_DMA_STREAM->PAR = (uint32_t)&ADC1->DR;

    /* 6. Memory address = our buffer */
    ADC_DMA_STREAM->M0AR = (uint32_t)buffer;

    /* 7. Number of data items */
    ADC_DMA_STREAM->NDTR = length;

    /* 8. Enable DMA2 Stream0 interrupt in NVIC
     *    Priority 1 — higher than SysTick (which is 15)
     *    so DMA ISR is never delayed by our 1ms tick
     */
    NVIC_SetPriority(DMA2_Stream0_IRQn, 1);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

void dma_adc_start(void)
{
    ADC_DMA_STREAM->CR |= DMA_SxCR_EN;
    while (!(ADC_DMA_STREAM->CR & DMA_SxCR_EN));
}

void dma_adc_stop(void)
{
    ADC_DMA_STREAM->CR &= ~DMA_SxCR_EN;
    while (ADC_DMA_STREAM->CR & DMA_SxCR_EN);
}

/* ── DMA2 Stream0 ISR ── */
void DMA2_Stream0_IRQHandler(void)
{
    /* Check LISR for Stream 0 flags */

    /* Transfer error — should not happen, but handle it */
    if (DMA2->LISR & DMA_LISR_TEIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTEIF0;
        /* In a real system: set an error flag, stop acquisition */
    }

    /* Half transfer */
    if (DMA2->LISR & DMA_LISR_HTIF0) {
        DMA2->LIFCR = DMA_LIFCR_CHTIF0;   /* clear flag */
        if (s_half_cb) s_half_cb();
    }

    /* Transfer complete */
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;   /* clear flag */
        if (s_full_cb) s_full_cb();
    }
}

