#ifndef TRIGGER_H
#define TRIGGER_H

#include <stdint.h>

/* Trigger modes */
#define TRIG_MODE_RISING   0U
#define TRIG_MODE_FALLING  1U
#define TRIG_MODE_AUTO     2U   /* fires after timeout if no edge found */

/* Trigger states */
#define TRIG_STATE_IDLE      0U
#define TRIG_STATE_ARMED     1U
#define TRIG_STATE_TRIGGERED 2U
#define TRIG_STATE_FROZEN    3U

/*
 * Captured frame after trigger fires.
 * Points into the capture buffer — not a copy.
 * Valid only while state == TRIG_STATE_FROZEN.
 */
typedef struct {
    uint16_t *samples;        /* pointer to first sample in frame  */
    uint16_t  count;          /* total samples in frame            */
    uint16_t  trigger_index;  /* index within samples[] of trigger */
    uint8_t   mode;           /* which trigger mode fired          */
} TriggerFrame_t;

/*
 * Trigger configuration.
 * Set before calling trigger_arm().
 */
typedef struct {
    uint8_t  mode;              /* TRIG_MODE_xx            */
    uint16_t threshold;         /* ADC counts, 0-4095      */
    uint16_t pre_trigger;       /* samples to keep before  */
    uint16_t post_trigger;      /* samples to capture after*/
    uint32_t auto_timeout_ms;   /* for TRIG_MODE_AUTO      */
} TriggerConfig_t;

void trigger_init   (uint16_t *work_buf, uint16_t buf_len);
void trigger_arm    (const TriggerConfig_t *cfg);
void trigger_reset  (void);

/*
 * Call this for every new sample from DMA callback.
 * Returns 1 when frozen (frame ready), 0 otherwise.
 */
uint8_t trigger_process(uint16_t sample, uint32_t timestamp_ms);

uint8_t              trigger_get_state(void);
const TriggerFrame_t *trigger_get_frame(void);

#endif
