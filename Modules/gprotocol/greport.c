/* Copyright © 2024 Georgy E. All rights reserved. */

#include "greport.h"

#include "glog.h"
#include "hal_defs.h"


static const char TAG[] = "GPTL";


uint16_t pack_crc(const pack_t* report)
{
    uint16_t crc = 0xFFFF;
    uint8_t* data = (uint8_t*)report;
    for (unsigned i = 0; i < sizeof(pack_t) - sizeof(report->crc); i++) {
        crc ^= (uint16_t)data[i];
        for (unsigned j = BITS_IN_BYTE; j != 0; j--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void pack_show(const char* key, const uint8_t index, const uint64_t data, const bool is_request, const bool is_get)
{

	if (is_request && is_get) {
		printTagLog(
			TAG,
			"%s: %s %s[%03u]",
			(is_request ? "Request " : "Response"),
			(is_get ? "get" : "set"),
			key,
			index
		);
	} else {
		printTagLog(
			TAG,
			"%s: %s %s[%03u] => 0x%08lX%08lX",
			(is_request ? "Request " : "Response"),
			(is_get ? "get" : "set"),
			key,
			index,
			(uint32_t)(data / 0x100000000),
			(uint32_t)(data % 0x100000000)
		);
	}
}
