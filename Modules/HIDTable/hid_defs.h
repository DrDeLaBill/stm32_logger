/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef HIDTABLE_HID_DEFS_H_
#define HIDTABLE_HID_DEFS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


#define HID_VENDOR_ID        (0x0483)
#define HID_PRODUCT_ID       (0xBEDA)

#define HID_INPUT_REPORT_ID  ((uint8_t)0xB1)
#define HID_OUTPUT_REPORT_ID ((uint8_t)0xB2)
#define HID_REPORT_SIZE      ((uint8_t)0x0A)

#define HID_GETTER_ID        ((uint8_t)0x00)


extern uint8_t receive_buf[HID_REPORT_SIZE + 1];


static const char REPORT_PREFIX[] = "LOG";


typedef struct __attribute__((packed)) _hid_report_t {
	uint8_t  report_id;
	uint16_t characteristic_id;
	uint8_t  index;
	uint8_t  data[HID_REPORT_SIZE - sizeof(uint8_t) * 2 - sizeof(uint16_t) - sizeof(REPORT_PREFIX)];
	uint8_t  tag[sizeof(REPORT_PREFIX)];
} hid_report_t;


#ifdef __cplusplus
}
#endif


#endif /* HIDTABLE_HID_DEFS_H_ */
