/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _GREPORT_H_
#define _GREPORT_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>

#include "gutils.h"


#define PACK_GETTER_KEY ((uint8_t)0x00)


#ifdef __MINGW32__
#   pragma pack(push, 1)
typedef struct
    _pack_t {
    uint32_t key;
    uint8_t  index;
    uint8_t  data[sizeof(uint64_t)];
    uint16_t crc;
} pack_t;
#   pragma pack(pop)
#else
TYPE_PACK(
    typedef struct,
    _pack_t {
        uint32_t key;
        uint8_t  index;
        uint8_t  data[sizeof(uint64_t)];
        uint16_t crc;
    } pack_t;
)
#endif


extern const char GPTL_TAG[];


uint16_t pack_crc(const pack_t* report);
void pack_show(
    const uint32_t key,
    const char* key_str,
    const uint8_t index,
    const uint64_t data,
    const bool is_request,
    const bool is_get
);


#ifdef __cplusplus
}
#endif


#endif
