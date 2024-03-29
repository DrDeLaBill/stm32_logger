/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _ONEWIRE_H_
#define _ONEWIRE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


void    onewire_proccess();

void    onewire_send_request(bool* data, uint16_t bitCount, uint16_t needBitCount);
void    onewire_read_bits(uint8_t count);
void    onewire_send_bit(uint8_t bit);
bool    onewire_result_ready();
bool*   onewire_response();
void    onewire_reset();
uint8_t onewire_crc8(uint8_t* data, uint8_t len);


#ifdef __cplusplus
}
#endif


#endif
