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


#ifdef __cplusplus
}
#endif


#endif /* HIDTABLE_HID_DEFS_H_ */
