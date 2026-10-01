#ifndef LOGIC_H
#define LOGIC_H

#include <stdint.h>

#define LOGIC_NUM_CHANNELS   2U
#define LOGIC_MAX_EDGES      256U   /* max edges per capture per channel */

typedef struct {
    uint32_t timestamp_us;   /* time of edge in microseconds */
    uint8_t  level;          /* level AFTER the edge (0 or 1) */
} LogicEdge_t;

typedef struct {
    LogicEdge_t edges[LOGIC_MAX_EDGES];
    uint16_t    count;
    uint8_t     channel;
} LogicChannel_t;

void logic_init(void);
void logic_start_capture(uint32_t duration_ms);
uint8_t logic_capture_done(void);

/* Returns pointer to channel data — valid after capture done */
const LogicChannel_t *logic_get_channel(uint8_t ch);

/* Send all captured channels as binary packet */
void logic_send_packet(void);

#endif
