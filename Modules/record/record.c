/* Copyright © 2024 Georgy E. All rights reserved. */

#include "record.h"

#include <stdio.h>

#include "soul.h"
#include "glog.h"
#include "clock.h"
#include "settings.h"
#include "internal_storage.h"


#define FILENAME_LENGTH (64)
#define STRING_LENGTH   (128)


#ifdef DEBUG
static const char RECORD_TAG[] = "RCD";
#endif
static const char RECORD_FILENAME[] = "dump.csv";



record_status_t record_save(const record_t* record)
{
	printTagLog(RECORD_TAG, "saving record");

	record_show(record);

	char filename[FILENAME_LENGTH] = {0};
	snprintf(filename, sizeof(filename) - 1, "%s" "%s", DIOSPIPath, RECORD_FILENAME);

	UINT br = 0;
	FRESULT res = FR_OK;
	char str[STRING_LENGTH] = {0};
	if (settings.record_id < 1) {
		snprintf(str, sizeof(str) - 1, "LOG_ID;TIME;MODBUS1_ID;MODBUS1_VALUE;1WIRE_ID;1WIRE_VALUE;\n");
		res = intstor_append_file(filename, &str, strlen(str), &br);
	}
	if(res != FR_OK) {
		printTagLog(RECORD_TAG, "record was NOT saved");
		return RECORD_ERROR;
	}


	for (unsigned i = 0; i < __max(record->mb1_count, record->_1w_count); i++) {
		if (i == 0) {
			snprintf(
				str,
				sizeof(str) - 1,
				"%lu;%s;%u;%d;%lu;%d;\n",
				record->id,
				get_clock_time_format(),
				(i < record->mb1_count) ? record->mb1_id[i] : 0,
				(i < record->mb1_count) ? record->mb1_value[i] : 0,
				(i < record->_1w_count) ? record->_1w_id[i] : 0,
				(i < record->_1w_count) ? record->_1w_value[i] : 0
			);
		} else {
			snprintf(
				str,
				sizeof(str) - 1,
				";;%u;%d;%lu;%d;\n",
				(i < record->mb1_count) ? record->mb1_id[i] : 0,
				(i < record->mb1_count) ? record->mb1_value[i] : 0,
				(i < record->_1w_count) ? record->_1w_id[i] : 0,
				(i < record->_1w_count) ? record->_1w_value[i] : 0
			);
		}

		res = intstor_append_file(filename, &str, strlen(str), &br);
		if(res != FR_OK) {
			printTagLog(RECORD_TAG, "record was NOT saved");
			set_error(SD_CARD_ERROR);
			return RECORD_ERROR;
		}
	}

	printTagLog(RECORD_TAG, "record saved");

	settings.record_id++;
	set_status(NEED_SAVE_SETTINGS);

	return RECORD_OK;
}

void record_show(const record_t* record)
{
	(void)record;
#if RECORD_BEDUG
    printPretty("############RECORD############\n");
    printPretty("Record ID: %lu\n", record->id);
    printPretty("Record time: %s\n", get_clock_time_format());
    printPretty("------------MODBUS1-----------\n");
    printPretty("INDEX ID                 VALUE\n");
    unsigned count = 0;
    while (count < record->mb1_count) {
		printPretty("%03u   %03u                %u\n", count, record->mb1_id[count], record->mb1_value[count]);
    	count++;
    }
    if (!count) {
        printPretty("------------EMPTY-------------\n");
    }
    printPretty("-------------1WIRE------------\n");
    printPretty("INDEX ID                 VALUE\n");
    count = 0;
    while (count < record->_1w_count) {
		printPretty("%03u   0x%08X%08X %u\n", count, (int)(record->_1w_id[count] >> 32), (int)(record->_1w_id[count]), record->_1w_value[count]);
    	count++;
    }
    if (!count) {
        printPretty("------------EMPTY-------------\n");
    }
    printPretty("############RECORD############\n");
#endif
}
