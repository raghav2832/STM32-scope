#include "stm32f4xx.h"
#include "bsp.h"
#include "systick.h"
#include "uart_driver.h"
#include "gpio_driver.h"
#include "timer_driver.h"
#include "adc_driver.h"
#include "dma_driver.h"
#include "trigger.h"
#include "protocol.h"
#include <stdio.h>

#define SAMPLE_RATE_HZ   100000U
#define BUFFER_SIZE      1024U
#define HALF_SIZE        (BUFFER_SIZE / 2U)
#define VREF_MV          3300U
#define PWM_FREQ_HZ      1000U

static volatile uint16_t adc_buf[BUFFER_SIZE];

#define TRIG_BUF_SIZE    512U
static uint16_t trig_buf[TRIG_BUF_SIZE];

static volatile uint8_t s_half_ready = 0;
static volatile uint8_t s_full_ready = 0;

static void on_half_transfer(void) { s_half_ready = 1; }
static void on_full_transfer(void) { s_full_ready = 1; }

static void rearm_trigger(void)
{
    TriggerConfig_t cfg = {
        .mode            = TRIG_MODE_RISING,
        .threshold       = 2048U,
        .pre_trigger     = 64U,
        .post_trigger    = 128U,
        .auto_timeout_ms = 500U
    };
    trigger_arm(&cfg);
}

static void feed_trigger(const volatile uint16_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint8_t frozen = trigger_process(buf[i], millis());
        if (frozen) {
            const TriggerFrame_t *frame = trigger_get_frame();

            /*
             * Send binary packet instead of printf text.
             * The trig_buf is the circular buffer inside trigger.c
             * We send the full frame: pre + post trigger samples.
             */
            proto_send_waveform(trig_buf,
                                frame->count,
                                frame->trigger_index,
                                frame->mode,
                                (uint16_t)(SAMPLE_RATE_HZ & 0xFFFFU),
                                VREF_MV);
            rearm_trigger();
            break;
        }
    }
}

int main(void)
{
    bsp_init();
    systick_init(SYS_CLK_HZ);
    uart_init(115200, APB1_CLK_HZ);

    /* Short text header before switching to binary */
    printf("\r\n=== M5: Binary Protocol ===\r\n");
    printf("Packets will now stream as binary.\r\n");
    printf("Open PC/main.py to visualize.\r\n\r\n");

    tim1_pwm_init(PWM_FREQ_HZ, 50);
    adc_init_dma(ADC_SAMPLETIME_56);
    dma_adc_init((uint16_t*)adc_buf, BUFFER_SIZE,
                 on_half_transfer,
                 on_full_transfer);

    trigger_init(trig_buf, TRIG_BUF_SIZE);
    rearm_trigger();

    dma_adc_start();
    tim2_init_trgo(SAMPLE_RATE_HZ);
    tim2_start();

    while (1)
    {
        if (s_half_ready) {
            s_half_ready = 0;
            feed_trigger(&adc_buf[0], HALF_SIZE);
        }
        if (s_full_ready) {
            s_full_ready = 0;
            feed_trigger(&adc_buf[HALF_SIZE], HALF_SIZE);
        }
    }
}
