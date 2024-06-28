/* Copyright © 2024 Georgy E. All rights reserved. */

#include "com_defs.h"

#include <string.h>

#ifdef USE_HAL_DRIVER
#   include "usbd_cdc_if.h"
#endif

#include "glog.h"
#include "gutils.h"
#include "bmacro.h"


uint16_t com_get_crc(const report_pack_t* report)
{
    uint16_t crc = 0xFFFF;
    uint8_t* data = (uint8_t*)report;
    for (unsigned i = 0; i < sizeof(report_pack_t) - sizeof(uint16_t); i++) {
        crc ^= (uint16_t)data[i];
        for (unsigned j = 8; j != 0; j--) {
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


#ifdef USE_HAL_DRIVER
bool com_get_report(report_pack_t* report)
{
	BEDUG_ASSERT(report, "Null COM report pointer");
	if (!report) {
		return false;
	}

	return false;
}


bool com_send_report(report_pack_t* report)
{
	BEDUG_ASSERT(report, "Null COM report pointer");
	if (!report) {
		return false;
	}

	report->flag = COM_SEND_FLAG;
	report->crc  = com_get_crc(report);

	return CDC_Transmit_FS((uint8_t*)report, sizeof(report_pack_t)) == USBD_OK;
}
#endif

void com_report_show(const report_pack_t* report)
{
    (void)report;
#if COM_TABLE_BEDUG
    printPretty("characteristic_id: %02u[%03u] => { ", report->characteristic_id, report->index);
    for (unsigned i = 0; i < __arr_len(report->data); i++) {
        gprint("%03u ", report->data[i]);
    }
    gprint("}\n");
#endif
}

void com_report_set_data(report_pack_t* report, const uint8_t* src_data, const unsigned size)
{
#if HID_TABLE_BEDUG
    BEDUG_ASSERT(src_data, "Data must not be null"); // TODO: app and device (throw)
#endif
    memset(report->data, 0, sizeof(report->data));
    memcpy(report->data, src_data, __min(sizeof(report->data), size));
}
