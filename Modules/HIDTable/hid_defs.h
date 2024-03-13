/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef HIDTABLE_HID_DEFS_H_
#define HIDTABLE_HID_DEFS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


#define HID_VENDOR_ID          (0x0483)
#define HID_PRODUCT_ID         (0xBEDA)

#define HID_OUTPUT_ENDPOINT    ((uint8_t)0x01)
#define HID_INPUT_ENDPOINT     ((uint8_t)0x81)
#define HID_INPUT_REPORT_ID    ((uint8_t)0xB1)
#define HID_OUTPUT_REPORT_ID   ((uint8_t)0xB2)
#define HID_REPORT_SIZE        ((uint8_t)0x0A)
#define HID_INTERFACE          (0)
#define HID_CONFIGURATION      (0)

#define HID_GETTER_ID          ((uint8_t)0x00)

#define HID_FIRST_KEY          ((uint16_t)1)

#define HID_DELAY_MS           (25)

#define HID_TABLE_BEDUG        (true)


static const char REPORT_PREFIX[] = { 'L', 'O', 'G' };


typedef struct __attribute__((packed)) _report_pack_t {
    uint8_t  report_id;
    uint16_t characteristic_id;
    uint8_t  index;
    uint8_t  data[sizeof(uint32_t)];
    uint8_t  tag[sizeof(REPORT_PREFIX)];
} report_pack_t;


void hid_report_show(const report_pack_t* report);
void hid_report_set_data(report_pack_t* report, const uint8_t* src_data, const unsigned size);



extern uint8_t receive_buf[sizeof(report_pack_t) + 1];


#ifdef __cplusplus
}
#endif


#endif /* HIDTABLE_HID_DEFS_H_ */
