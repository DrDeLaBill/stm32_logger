/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef __USB_HID_TABLE_H
#define __USB_HID_TABLE_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


#define INPUT_REPORT_ID  ((uint8_t)0xB1)
#define OUTPUT_REPORT_ID ((uint8_t)0xB2)


typedef struct _DESCRIPTOR_UNIT {
	uint8_t* ptr;
	uint8_t  size;
} DESCRIPTOR_UNIT;


static DESCRIPTOR_UNIT USB_HID_TABLE[] = {
	{},
	{}
};


#ifdef __cplusplus
}
#endif


#endif
