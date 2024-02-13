/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef HIDTABLE_HID_DEFS_H_
#define HIDTABLE_HID_DEFS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "log.h"
#include "utils.h"
#include "bmacro.h"


#define HID_VENDOR_ID        (0x0483)
#define HID_PRODUCT_ID       (0xBEDA)

#define HID_OUTPUT_ENDPOINT  ((uint8_t)0x01)
#define HID_INPUT_ENDPOINT   ((uint8_t)0x81)
#define HID_INPUT_REPORT_ID  ((uint8_t)0xB1)
#define HID_OUTPUT_REPORT_ID ((uint8_t)0xB2)
#define HID_REPORT_SIZE      ((uint8_t)0x0A)
#define HID_INTERFACE        (0)
#define HID_CONFIGURATION    (0)

#define HID_GETTER_ID        ((uint8_t)0x00)

#define HID_FIRST_KEY        ((uint16_t)1)

#define HID_DELAY_MS         (25)


static const char REPORT_PREFIX[] = { 'L', 'O', 'G' };


typedef struct __attribute__((packed)) _report_pack_t {
    uint8_t  report_id;
    uint16_t characteristic_id;
    uint8_t  index;
    uint8_t  data[sizeof(uint32_t)];
    uint8_t  tag[sizeof(REPORT_PREFIX)];

#ifndef USE_HAL_DRIVER
    void show()
    {
        printPretty("report_id: %u\n", report_id);
        printPretty("characteristic_id: %u\n", characteristic_id);
        printPretty("index: %u\n", index);
        printPretty("data: %u %u %u %u\n", data[0], data[1], data[2], data[3]);
    }

    void setData(const uint8_t* src_data, const unsigned size)
    {
        BEDUG_ASSERT(src_data, "Data must not be null"); // TODO: app and device (throw)
        memcpy(data, src_data, __min(sizeof(data), size));
    }
#endif
} report_pack_t;


extern uint8_t receive_buf[sizeof(report_pack_t) + 1];


#ifdef __cplusplus
}
#endif


#endif /* HIDTABLE_HID_DEFS_H_ */
