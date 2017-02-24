#ifndef GPIO_H
#define GPIO_H

#include <xparameters.h>
#include <xil_types.h>

typedef struct {
    u32* reg_addr;
    int bit_nr;
    u32* reg_cache; // use with AXI GPIO which doesn't allow read back of the out register
} gpio_bit;

#define GPIO_REG ((u32*) (XPAR_AXI_GPIO_0_BASEADDR +  0 * 4))

u32 bit_modify(u32 x, int nr, int value);
int bit_get(u32 x, int nr);
int gpio_in(gpio_bit *g);
void gpio_out(gpio_bit *g, int value);
void gpio_toggle(gpio_bit *g);
void wait_nop_approx_milliseconds(u32 ms);

extern gpio_bit led_r_green;
extern gpio_bit led_r_red;
extern gpio_bit led_m_green;
extern gpio_bit led_m_red;
extern gpio_bit led_l_green;
extern gpio_bit led_l_red;
extern gpio_bit gpio_scl;
extern gpio_bit gpio_sda;

#endif // GPIO_H
