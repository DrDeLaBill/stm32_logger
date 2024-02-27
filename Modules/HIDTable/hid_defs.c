/* Copyright © 2024 Georgy E. All rights reserved. */

#include "hid_defs.h"

#include <string.h>

#include "log.h"
#include "utils.h"
#include "bmacro.h"


void hid_report_show(const report_pack_t* report)
{
    printPretty("report_id:         %u\n", report->report_id);
    printPretty("characteristic_id: %02u[%03u] => { %03u %03u %03u %03u }\n", report->characteristic_id, report->index, report->data[0], report->data[1], report->data[2], report->data[3]);
}

void hid_report_set_data(report_pack_t* report, const uint8_t* src_data, const unsigned size)
{
    BEDUG_ASSERT(src_data, "Data must not be null"); // TODO: app and device (throw)

    memcpy_s(report->data, sizeof(report->data), src_data, size);
}
