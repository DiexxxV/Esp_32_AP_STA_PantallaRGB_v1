#include "io_ctrl.h"
#include "driver/gpio.h"

#define RELAY1 GPIO_NUM_16
#define RELAY2 GPIO_NUM_2

#define INPUT1 GPIO_NUM_34
#define INPUT2 GPIO_NUM_32



void io_init(void)
{
    gpio_config_t io = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RELAY1) | (1ULL << RELAY2)
    };
    gpio_config(&io);

    gpio_config_t in = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << INPUT1) | (1ULL << INPUT2)
    };
    gpio_config(&in);

    gpio_set_level(RELAY1, 0);
    gpio_set_level(RELAY2, 0);
}

void io_set_relay(uint8_t relay, uint8_t state)
{
    if (relay == 1) gpio_set_level(RELAY1, state);
    if (relay == 2) gpio_set_level(RELAY2, state);
}

int io_get_input(uint8_t input)
{
    if (input == 1) return gpio_get_level(INPUT1);
    if (input == 2) return gpio_get_level(INPUT2);
    return -1;
}

