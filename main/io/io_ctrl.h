
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void io_init(void);
void io_set_relay(uint8_t relay, uint8_t state);
int  io_get_input(uint8_t input);
