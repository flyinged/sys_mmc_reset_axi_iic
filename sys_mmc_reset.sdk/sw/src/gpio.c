#include "gpio.h"
#include <assert.h>

u32 gpio_reg_cache = 0;
gpio_bit led_r_green = {GPIO_REG, 0, &gpio_reg_cache};
gpio_bit led_r_red   = {GPIO_REG, 1, &gpio_reg_cache};
gpio_bit led_m_green = {GPIO_REG, 2, &gpio_reg_cache};
gpio_bit led_m_red   = {GPIO_REG, 3, &gpio_reg_cache};
gpio_bit led_l_green = {GPIO_REG, 4, &gpio_reg_cache};
gpio_bit led_l_red   = {GPIO_REG, 5, &gpio_reg_cache};

u32 bit_modify(u32 x, int nr, int value)
{
    assert((nr >= 0) && (nr <= 31));
    return value ? (x | (1 << nr)) : (x & ~(1 << nr));
}

int bit_get(u32 x, int nr)
{
    assert((nr >= 0) && (nr <= 31));
    return (x & (1 << nr)) ? 1 : 0;
}

int gpio_in(gpio_bit *g)
{
    if (g->reg_cache)
    {
        return bit_get(*(g->reg_cache), g->bit_nr);
    }
    else
    {
        return bit_get(*(g->reg_addr), g->bit_nr);
    }
}

void gpio_out(gpio_bit *g, int value)
{
    u32 out = 0;

    if (g->reg_cache)
    {
        out = bit_modify(*(g->reg_cache), g->bit_nr, value);
        *(g->reg_cache) = out;
        *(g->reg_addr)  = out;
    }
    else
    {
        *(g->reg_addr) = bit_modify(*(g->reg_addr), g->bit_nr, value);
    }
}

void gpio_toggle(gpio_bit *g)
{
    gpio_out(g, !gpio_in(g));
}

void wait_nop_approx_milliseconds(u32 ms)
{
	u32 i = 0;
	u32 limit = 0;

	limit = 10000 * ms;

	for (i = 0; i < limit; i++) {
		asm("nop");
    }
}
