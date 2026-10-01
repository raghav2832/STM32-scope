#ifndef BSP_H
#define BSP_H

#include <stdint.h>

#define SYS_CLK_HZ    180000000UL
#define APB1_CLK_HZ    45000000UL
#define APB2_CLK_HZ    90000000UL

void bsp_init(void);

#endif
