#include "exti_driver.h"
#include "gpio_driver.h"

/* One callback per EXTI line (pins 0-15) */
static exti_callback_t s_callbacks[16] = {0};

/* Port pointers so ISR can read pin level */
static GPIO_TypeDef   *s_ports[16]     = {0};

void exti_init(uint8_t pin,
               uint8_t port_sel,
               uint8_t edge,
               exti_callback_t cb)
{
    /* 1. Resolve GPIO port pointer */
    GPIO_TypeDef *port_bases[] = {
        GPIOA, GPIOB, GPIOC, GPIOD, GPIOE
    };
    GPIO_TypeDef *port = port_bases[port_sel];

    s_callbacks[pin] = cb;
    s_ports[pin]     = port;

    /* 2. Configure pin as input with pull-down */
    GPIO_PinCfg_t cfg = {
        .port  = port,
        .pin   = pin,
        .mode  = GPIO_MODE_INPUT,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_HIGH,
        .pupd  = GPIO_PUPD_DOWN,
        .af    = 0
    };
    gpio_init(&cfg);

    /* 3. Enable SYSCFG clock (needed for EXTICR) */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    volatile uint32_t dummy = RCC->APB2ENR;
    (void)dummy;

    /*
     * 4. SYSCFG_EXTICRx: select which port drives this EXTI line
     *    Each register has 4 fields of 4 bits
     *    EXTICR[0] = pins 0-3
     *    EXTICR[1] = pins 4-7
     *    EXTICR[2] = pins 8-11
     *    EXTICR[3] = pins 12-15
     *    Field position within register = (pin % 4) * 4
     */
    uint8_t  reg = pin / 4U;
    uint8_t  pos = (pin % 4U) * 4U;
    SYSCFG->EXTICR[reg] &= ~(0xFU << pos);
    SYSCFG->EXTICR[reg] |=  (port_sel << pos);

    /* 5. Unmask this EXTI line */
    EXTI->IMR |= (1U << pin);

    /* 6. Set edge trigger */
    if (edge == EXTI_EDGE_RISING || edge == EXTI_EDGE_BOTH)
        EXTI->RTSR |= (1U << pin);
    if (edge == EXTI_EDGE_FALLING || edge == EXTI_EDGE_BOTH)
        EXTI->FTSR |= (1U << pin);

    /* 7. Enable NVIC interrupt
     *    EXTI0 and EXTI1 have dedicated IRQs.
     *    EXTI2 and EXTI3 have dedicated IRQs.
     *    EXTI9_5 covers pins 5-9.
     *    EXTI15_10 covers pins 10-15.
     */
    IRQn_Type irqn;
    if      (pin == 0) irqn = EXTI0_IRQn;
    else if (pin == 1) irqn = EXTI1_IRQn;
    else if (pin == 2) irqn = EXTI2_IRQn;
    else if (pin == 3) irqn = EXTI3_IRQn;
    else if (pin == 4) irqn = EXTI4_IRQn;
    else if (pin <= 9) irqn = EXTI9_5_IRQn;
    else               irqn = EXTI15_10_IRQn;

    NVIC_SetPriority(irqn, 2U);
    NVIC_EnableIRQ(irqn);
}

/* ── Generic EXTI handler — call from specific ISR ── */
static void exti_handle(uint8_t pin)
{
    if (EXTI->PR & (1U << pin)) {
        EXTI->PR = (1U << pin);   /* clear pending — write 1 to clear */

        if (s_callbacks[pin] && s_ports[pin]) {
            uint8_t level = gpio_read(s_ports[pin], pin);
            s_callbacks[pin](pin, level);
        }
    }
}

/* ── ISR definitions ── */
void EXTI0_IRQHandler(void) { exti_handle(0); }
void EXTI1_IRQHandler(void) { exti_handle(1); }
void EXTI2_IRQHandler(void) { exti_handle(2); }
void EXTI3_IRQHandler(void) { exti_handle(3); }
void EXTI4_IRQHandler(void) { exti_handle(4); }

void EXTI9_5_IRQHandler(void)
{
    for (uint8_t p = 5; p <= 9; p++)
        exti_handle(p);
}

void EXTI15_10_IRQHandler(void)
{
    for (uint8_t p = 10; p <= 15; p++)
        exti_handle(p);
}
