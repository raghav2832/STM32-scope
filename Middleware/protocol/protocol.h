#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "trigger.h"

/* Sync bytes */
#define PROTO_SYNC_0        0xAAU
#define PROTO_SYNC_1        0x55U

/* Packet types */
#define PROTO_TYPE_WAVEFORM 0x01U
#define PROTO_TYPE_LOGIC    0x02U
#define PROTO_TYPE_ACK      0x03U
#define PROTO_TYPE_ERROR    0x04U

/* Header size: 2 sync + 1 type + 1 seq + 2 len = 6 bytes */
#define PROTO_HEADER_SIZE   6U
/* Footer size: 2 CRC bytes */
#define PROTO_FOOTER_SIZE   2U

/*
 * Send a waveform frame over UART.
 * samples     = pointer to sample array
 * num_samples = how many uint16_t samples
 * trig_idx    = index of trigger point in samples[]
 * trig_mode   = TRIG_MODE_xx
 * sample_rate = Hz
 * vref_mv     = reference voltage in millivolts (3300)
 */
void proto_send_waveform(const uint16_t *samples,
                         uint16_t        num_samples,
                         uint16_t        trig_idx,
                         uint8_t         trig_mode,
                         uint16_t        sample_rate_hz,
                         uint16_t        vref_mv);

void proto_send_ack  (uint8_t seq);
void proto_send_error(uint8_t code);

#endif
