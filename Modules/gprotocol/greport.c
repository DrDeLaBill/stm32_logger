/* Copyright © 2024 Georgy E. All rights reserved. */

#include "greport.h"

#include "glog.h"
#include "gutils.h"


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

void pack_show(const pack_t* report)
{
    printPretty("key: %02lu[%03u] => { ", report->key, report->index);
    for (unsigned i = 0; i < __arr_len(report->data); i++) {
        gprint("%03u ", report->data[i]);
    }
    gprint("}\n");
}
