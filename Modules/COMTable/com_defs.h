/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _COM_DEFS_H_
#define _COM_DEFS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define COM_PRODUCT_ID         (0xBEDA)

#define COM_INTERFACE          (0)
#define COM_CONFIGURATION      (0)

#define COM_SEND_FLAG          ((uint8_t)0x01)
#define COM_GETTER_ID          ((uint8_t)0x00)

#define COM_FIRST_KEY          ((uint16_t)1)

#define COM_DELAY_MS           (25)

#define COM_TABLE_BEDUG        (true)


static const char COM_TAG[] = "COM";


typedef struct __attribute__((packed)) _report_pack_t {
	uint8_t  flag;
    uint16_t characteristic_id;
    uint8_t  index;
    uint8_t  data[sizeof(uint32_t)];
    uint16_t crc;
} report_pack_t;


uint16_t com_get_crc(const report_pack_t* report);

#ifdef USE_HAL_DRIVER
bool com_get_report(report_pack_t* report);
bool com_send_report(report_pack_t* report);
#endif

void com_report_show(const report_pack_t* report);


#ifdef __cplusplus
}
#endif


#endif /* HIDTABLE_COM_DEFS_H_ */
