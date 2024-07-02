/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _GREPORT_H_
#define _GREPORT_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>


#define PACK_GETTER_KEY ((uint8_t)0x00)


typedef struct __attribute__((packed)) _pack_t {
    uint32_t key;
    uint8_t  index;
    uint8_t  data[sizeof(uint64_t)];
    uint16_t crc;
} pack_t;


uint16_t pack_crc(const pack_t* report);
void pack_show(const pack_t* report);


#ifdef __cplusplus
}
#endif


#endif
