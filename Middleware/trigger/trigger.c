#include "trigger.h"
#include <string.h>

/* Internal state */
static uint8_t         s_state       = TRIG_STATE_IDLE;
static TriggerConfig_t s_cfg         = {0};
static TriggerFrame_t  s_frame       = {0};

/* Circular capture buffer (provided by caller) */
static uint16_t       *s_buf         = 0;
static uint16_t        s_buf_len     = 0;
static uint16_t        s_write_idx   = 0;   /* next write position  */
static uint16_t        s_filled      = 0;   /* total samples written */

/* Post-trigger counter */
static uint16_t        s_post_count  = 0;

/* Auto-trigger timeout */
static uint32_t        s_arm_time_ms = 0;

/* Previous sample — needed for edge detection */
static uint16_t        s_prev_sample = 0;

/* ── Internal helpers ── */

static void buf_write(uint16_t sample)
{
    s_buf[s_write_idx] = sample;
    s_write_idx = (s_write_idx + 1U) % s_buf_len;
    if (s_filled < s_buf_len)
        s_filled++;
}

/*
 * After trigger fires, the pre-trigger samples are already
 * in the circular buffer behind s_write_idx.
 * This function finds the start index of the frame.
 */

/* ── Public API ── */

void trigger_init(uint16_t *work_buf, uint16_t buf_len)
{
    s_buf      = work_buf;
    s_buf_len  = buf_len;
    s_state    = TRIG_STATE_IDLE;
    s_write_idx = 0;
    s_filled    = 0;
}

void trigger_arm(const TriggerConfig_t *cfg)
{
    s_cfg         = *cfg;
    s_state       = TRIG_STATE_ARMED;
    s_post_count  = 0;
    s_prev_sample = 2048U;   /* start at mid-scale, not 0 */
    s_arm_time_ms = 0;
}

void trigger_reset(void)
{
    s_state      = TRIG_STATE_IDLE;
    s_write_idx  = 0;
    s_filled     = 0;
    s_post_count = 0;
}

uint8_t trigger_process(uint16_t sample, uint32_t timestamp_ms)
{
    if (s_state == TRIG_STATE_IDLE || s_state == TRIG_STATE_FROZEN)
        return (s_state == TRIG_STATE_FROZEN) ? 1U : 0U;

    /* Record arm time on first sample */
    if (s_arm_time_ms == 0U)
        s_arm_time_ms = timestamp_ms;

    /* Always write sample into circular buffer */
    buf_write(sample);

    if (s_state == TRIG_STATE_ARMED) {

        uint8_t fired = 0;

        /* Check trigger condition */
        if (s_cfg.mode == TRIG_MODE_RISING) {
            /*
             * Rising edge: previous sample below threshold,
             * current sample at or above threshold.
             */
            if ((s_prev_sample < s_cfg.threshold) &&
                (sample        >= s_cfg.threshold)) {
                fired = 1;
            }
        }
        else if (s_cfg.mode == TRIG_MODE_FALLING) {
            /*
             * Falling edge: previous sample above threshold,
             * current sample at or below threshold.
             */
            if ((s_prev_sample > s_cfg.threshold) &&
                (sample        <= s_cfg.threshold)) {
                fired = 1;
            }
        }
        else if (s_cfg.mode == TRIG_MODE_AUTO) {
            /* Rising edge first, fall back to timeout */
            if ((s_prev_sample < s_cfg.threshold) &&
                (sample        >= s_cfg.threshold)) {
                fired = 1;
            }
            else if ((timestamp_ms - s_arm_time_ms) >= s_cfg.auto_timeout_ms) {
                fired = 1;   /* timeout — capture whatever we have */
            }
        }

        if (fired) {
            s_state      = TRIG_STATE_TRIGGERED;
            s_post_count = 0;

            /*
             * Record where in the circular buffer the trigger sample
             * landed. write_idx already advanced past it, so trigger
             * is at (write_idx - 1 + buf_len) % buf_len.
             */
            uint16_t trig_buf_idx =
                (s_write_idx + s_buf_len - 1U) % s_buf_len;

            /*
             * Frame start: go back pre_trigger samples from trigger.
             * pre_trigger_start is the oldest sample we want.
             */
            uint16_t pre      = s_cfg.pre_trigger;
            uint16_t avail    = (s_filled < pre) ? s_filled : pre;
            uint16_t frame_start =
                (trig_buf_idx + s_buf_len - avail) % s_buf_len;

            s_frame.samples       = s_buf;
            s_frame.trigger_index = avail;   /* offset into frame */
            s_frame.mode          = s_cfg.mode;
            /* count filled in when FROZEN */
            (void)frame_start;
        }
    }
    else if (s_state == TRIG_STATE_TRIGGERED) {
        s_post_count++;

        if (s_post_count >= s_cfg.post_trigger) {
            /* Enough post-trigger samples collected → freeze */
            s_state        = TRIG_STATE_FROZEN;
            s_frame.count  = s_cfg.pre_trigger + s_cfg.post_trigger;
        }
    }

    s_prev_sample = sample;
    return (s_state == TRIG_STATE_FROZEN) ? 1U : 0U;
}

uint8_t trigger_get_state(void)
{
    return s_state;
}

const TriggerFrame_t *trigger_get_frame(void)
{
    return &s_frame;
}
