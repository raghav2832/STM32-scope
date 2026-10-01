#include "protocol.h"
#include "uart_driver.h"
#include "crc16.h"

/* Sequence number — increments with each packet sent */
static uint8_t s_seq = 0;

/*
 * Send raw byte — single point where all output goes.
 * Swap uart_send_byte() for DMA TX later if needed.
 */
static void send_byte(uint8_t b)
{
    uart_send_byte(b);
}

/*
 * Send a complete packet:
 *   header → payload → CRC
 *
 * CRC covers: type + seq + len_l + len_h + payload
 * Does NOT cover sync bytes (they are outside the protected frame)
 */
static void send_packet(uint8_t         type,
                        const uint8_t  *payload,
                        uint16_t        payload_len)
{
    uint16_t crc = 0xFFFFU;

    /* ── Sync bytes (not CRC'd) ── */
    send_byte(PROTO_SYNC_0);
    send_byte(PROTO_SYNC_1);

    /* ── Header (CRC'd from here) ── */
    send_byte(type);
    crc = crc16_update(crc, type);

    send_byte(s_seq);
    crc = crc16_update(crc, s_seq);
    s_seq++;

    uint8_t len_l = (uint8_t)(payload_len & 0xFFU);
    uint8_t len_h = (uint8_t)(payload_len >> 8U);

    send_byte(len_l);
    crc = crc16_update(crc, len_l);

    send_byte(len_h);
    crc = crc16_update(crc, len_h);

    /* ── Payload (CRC'd) ── */
    for (uint16_t i = 0; i < payload_len; i++) {
        send_byte(payload[i]);
        crc = crc16_update(crc, payload[i]);
    }

    /* ── CRC footer ── */
    send_byte((uint8_t)(crc & 0xFFU));
    send_byte((uint8_t)(crc >> 8U));
}

void proto_send_waveform(const uint16_t *samples,
                         uint16_t        num_samples,
                         uint16_t        trig_idx,
                         uint8_t         trig_mode,
                         uint16_t        sample_rate_hz,
                         uint16_t        vref_mv)
{
    /*
     * Payload layout:
     *   [0]  sample_rate low
     *   [1]  sample_rate high
     *   [2]  trig_idx low
     *   [3]  trig_idx high
     *   [4]  trig_mode
     *   [5]  vref_mv low
     *   [6]  vref_mv high
     *   [7..] samples, 2 bytes each, little-endian
     *
     * We build the 7-byte metadata header inline,
     * then stream samples directly — no giant stack buffer.
     */

    uint16_t payload_len = 7U + (num_samples * 2U);
    uint16_t crc         = 0xFFFFU;

    /* Sync */
    send_byte(PROTO_SYNC_0);
    send_byte(PROTO_SYNC_1);

    /* Type */
    send_byte(PROTO_TYPE_WAVEFORM);
    crc = crc16_update(crc, PROTO_TYPE_WAVEFORM);

    /* Seq */
    send_byte(s_seq);
    crc = crc16_update(crc, s_seq);
    s_seq++;

    /* Length */
    uint8_t len_l = (uint8_t)(payload_len & 0xFFU);
    uint8_t len_h = (uint8_t)(payload_len >> 8U);
    send_byte(len_l); crc = crc16_update(crc, len_l);
    send_byte(len_h); crc = crc16_update(crc, len_h);

    /* Metadata */
    uint8_t sr_l = (uint8_t)(sample_rate_hz & 0xFFU);
    uint8_t sr_h = (uint8_t)(sample_rate_hz >> 8U);
    send_byte(sr_l); crc = crc16_update(crc, sr_l);
    send_byte(sr_h); crc = crc16_update(crc, sr_h);

    uint8_t ti_l = (uint8_t)(trig_idx & 0xFFU);
    uint8_t ti_h = (uint8_t)(trig_idx >> 8U);
    send_byte(ti_l); crc = crc16_update(crc, ti_l);
    send_byte(ti_h); crc = crc16_update(crc, ti_h);

    send_byte(trig_mode); crc = crc16_update(crc, trig_mode);

    uint8_t vr_l = (uint8_t)(vref_mv & 0xFFU);
    uint8_t vr_h = (uint8_t)(vref_mv >> 8U);
    send_byte(vr_l); crc = crc16_update(crc, vr_l);
    send_byte(vr_h); crc = crc16_update(crc, vr_h);

    /* Samples — stream directly, no copy */
    for (uint16_t i = 0; i < num_samples; i++) {
        uint8_t s_l = (uint8_t)(samples[i] & 0xFFU);
        uint8_t s_h = (uint8_t)(samples[i] >> 8U);
        send_byte(s_l); crc = crc16_update(crc, s_l);
        send_byte(s_h); crc = crc16_update(crc, s_h);
    }

    /* CRC footer */
    send_byte((uint8_t)(crc & 0xFFU));
    send_byte((uint8_t)(crc >> 8U));
}

void proto_send_ack(uint8_t seq)
{
    uint8_t payload = seq;
    send_packet(PROTO_TYPE_ACK, &payload, 1U);
}

void proto_send_error(uint8_t code)
{
    uint8_t payload = code;
    send_packet(PROTO_TYPE_ERROR, &payload, 1U);
}
