#include "gpio_driver.h"

static void gpio_clock_enable(GPIO_TypeDef *port)
{
    uint32_t bit = ((uint32_t)port - (uint32_t)GPIOA_BASE) / 0x400U;
    RCC->AHB1ENR |= (1U << bit);
    volatile uint32_t dummy = RCC->AHB1ENR;
    (void)dummy;
}

void gpio_init(const GPIO_PinCfg_t *cfg)
{
    gpio_clock_enable(cfg->port);

    /* Mode */
    cfg->port->MODER &= ~(3U << (cfg->pin * 2U));
    cfg->port->MODER |=  (cfg->mode << (cfg->pin * 2U));

    /* Output type */
    cfg->port->OTYPER &= ~(1U << cfg->pin);
    cfg->port->OTYPER |=  (cfg->otype << cfg->pin);

    /* Speed */
    cfg->port->OSPEEDR &= ~(3U << (cfg->pin * 2U));
    cfg->port->OSPEEDR |=  (cfg->speed << (cfg->pin * 2U));

    /* Pull */
    cfg->port->PUPDR &= ~(3U << (cfg->pin * 2U));
    cfg->port->PUPDR |=  (cfg->pupd << (cfg->pin * 2U));

    /* Alternate function */
    if (cfg->mode == GPIO_MODE_AF) {
        uint8_t reg = cfg->pin / 8U;
        uint8_t pos = (cfg->pin % 8U) * 4U;
        cfg->port->AFR[reg] &= ~(0xFU << pos);
        cfg->port->AFR[reg] |=  (cfg->af << pos);
    }
}

void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t val)
{
    if (val)
        port->BSRR = (1U << pin);
    else
        port->BSRR = (1U << (pin + 16U));
}

void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->ODR ^= (1U << pin);
}

uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR >> pin) & 1U;
}
