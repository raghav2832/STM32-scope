#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/* MODER — 2 bits per pin */
#define GPIO_MODE_INPUT     0x0U
#define GPIO_MODE_OUTPUT    0x1U
#define GPIO_MODE_AF        0x2U
#define GPIO_MODE_ANALOG    0x3U

/* OTYPER — 1 bit per pin */
#define GPIO_OTYPE_PP       0x0U
#define GPIO_OTYPE_OD       0x1U

/* PUPDR — 2 bits per pin */
#define GPIO_PUPD_NONE      0x0U
#define GPIO_PUPD_UP        0x1U
#define GPIO_PUPD_DOWN      0x2U

/* OSPEEDR — 2 bits per pin */
#define GPIO_SPEED_LOW      0x0U
#define GPIO_SPEED_MED      0x1U
#define GPIO_SPEED_HIGH     0x2U
#define GPIO_SPEED_VHIGH    0x3U

typedef struct {
    GPIO_TypeDef *port;
    uint8_t       pin;
    uint8_t       mode;
    uint8_t       otype;
    uint8_t       speed;
    uint8_t       pupd;
    uint8_t       af;
} GPIO_PinCfg_t;

void    gpio_init  (const GPIO_PinCfg_t *cfg);
void    gpio_write (GPIO_TypeDef *port, uint8_t pin, uint8_t val);
void    gpio_toggle(GPIO_TypeDef *port, uint8_t pin);
uint8_t gpio_read  (GPIO_TypeDef *port, uint8_t pin);

#endif
