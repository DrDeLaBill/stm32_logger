/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _ONEWIRE_DRIVER_H_
#define _ONEWIRE_DRIVER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


void     onewire_driver_tick();

bool     onewire_driver_ready();
void     onewire_driver_clear();

void     onewire_driver_start_search();
void     onewire_driver_next_search();
void     onewire_driver_start_read(uint64_t address);
int16_t  get_onewire_driver_value();
uint64_t get_onewire_driver_address();


#ifdef __cplusplus
}
#endif


#endif
