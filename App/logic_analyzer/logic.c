#include "logic.h"
#include "exti_driver.h"
#include "protocol.h"
#include "uart_driver.h"
#include "crc16.h"
#include "systick.h"
#include "stm32f4xx.h"

/*
 * TIM5 as free-running 32-bit microsecond counter.
 * We read TIM5->CNT at each EXTI interrupt for timestamp.
 * TIM5 is on APB1. TIM5 clock = 90MHz.
 * PSC = 89 → tick = 90MHz/90 = 1MHz → 1us per tick.
 */

static LogicChannel_t s_channels[LOGIC_NUM_CHANNELS];
static volatile uint8_t  s_capturing  = 0;
static uint32_t          s_stop_time  = 0;

/* ── TIM5 microsecond counter ── */
static void tim5_us_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    volatile uint32_t dummy = RCC->APB1ENR;
    (void)dummy;

    TIM5->CR1  = 0;
    TIM5->PSC  = 89U;          /* 90MHz / 90 = 1MHz = 1us tick */
    TIM5->ARR  = 0xFFFFFFFFU;  /* 32-bit free run */
    TIM5->EGR  = TIM_EGR_UG;
    TIM5->CR1 |= TIM_CR1_CEN;
}

static uint32_t get_us(void)
{
    return TIM5->CNT;
}

/* ── EXTI callback — called from ISR ── */
static void on_edge(uint8_t pin, uint8_t level)
{
    if (!s_capturing) return;

    uint8_t ch = 0xFF;
    if (pin == 0) ch = 0;   /* PB0 = channel 0 */
    if (pin == 1) ch = 1;   /* PB1 = channel 1 */
    if (ch == 0xFF) return;

    LogicChannel_t *c = &s_channels[ch];
    if (c->count >= LOGIC_MAX_EDGES) return;

    c->edges[c->count].timestamp_us = get_us();
    c->edges[c->count].level        = level;
    c->count++;
}

void logic_init(void)
{
    tim5_us_init();

    /* Configure PB0, PB1 as EXTI inputs, both edges */
    exti_init(0, EXTI_PORT_B, EXTI_EDGE_BOTH, on_edge);
    exti_init(1, EXTI_PORT_B, EXTI_EDGE_BOTH, on_edge);
}

void logic_start_capture(uint32_t duration_ms)
{
    /* Reset buffers */
    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        s_channels[ch].count   = 0;
        s_channels[ch].channel = ch;
    }

    s_stop_time = millis() + duration_ms;
    s_capturing = 1;
}

uint8_t logic_capture_done(void)
{
    if (!s_capturing) return 0;
    if (millis() >= s_stop_time) {
        s_capturing = 0;
        return 1;
    }
    return 0;
}

const LogicChannel_t *logic_get_channel(uint8_t ch)
{
    if (ch >= LOGIC_NUM_CHANNELS) return 0;
    return &s_channels[ch];
}

/*
 * Logic packet format (type 0x02):
 *
 * Header (after proto sync+type+seq+len):
 *   [0]  num_channels  uint8
 *   [1]  duration_ms L uint8
 *   [2]  duration_ms H uint8
 *
 * Per channel block:
 *   [0]  channel_id   uint8
 *   [1]  edge_count L uint8
 *   [2]  edge_count H uint8
 *   Per edge (5 bytes each):
 *     [0..3] timestamp_us uint32 LE
 *     [4]    level        uint8
 */
void logic_send_packet(void)
{
    /* Build payload in a local buffer */
    static uint8_t buf[16 + LOGIC_NUM_CHANNELS *
                        (3 + LOGIC_MAX_EDGES * 5)];
    uint16_t idx = 0;

    uint32_t duration = s_stop_time - (s_stop_time - 100);
    buf[idx++] = LOGIC_NUM_CHANNELS;
    buf[idx++] = 100U & 0xFF;   /* duration low  */
    buf[idx++] = 100U >> 8;     /* duration high */

    for (uint8_t ch = 0; ch < LOGIC_NUM_CHANNELS; ch++) {
        const LogicChannel_t *c = &s_channels[ch];
        buf[idx++] = ch;
        buf[idx++] = (uint8_t)(c->count & 0xFF);
        buf[idx++] = (uint8_t)(c->count >> 8);

        for (uint16_t e = 0; e < c->count; e++) {
            uint32_t ts = c->edges[e].timestamp_us;
            buf[idx++] = (uint8_t)(ts        & 0xFF);
            buf[idx++] = (uint8_t)((ts >> 8) & 0xFF);
            buf[idx++] = (uint8_t)((ts >>16) & 0xFF);
            buf[idx++] = (uint8_t)((ts >>24) & 0xFF);
            buf[idx++] = c->edges[e].level;
        }
    }

    /* Send as protocol packet type 0x02 */
    uint16_t crc = crc16_buf(buf, idx);

    uart_send_byte(0xAAU);
    uart_send_byte(0x55U);

    static uint8_t seq = 0;
    uint8_t hdr[4] = {
        0x02,           /* PROTO_TYPE_LOGIC */
        seq++,
        (uint8_t)(idx & 0xFF),
        (uint8_t)(idx >> 8)
    };
    uart_send_buf(hdr, 4);
    uart_send_buf(buf, idx);
    uart_send_byte((uint8_t)(crc & 0xFF));
    uart_send_byte((uint8_t)(crc >> 8));
}
