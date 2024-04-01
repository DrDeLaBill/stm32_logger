/* Copyright © 2024 Georgy E. All rights reserved. */

#include "record.h"

#include <string.h>

#include "log.h"
#include "clock.h"
#include "bmacro.h"
#include "settings.h"

#include "StorageType.h"


unsigned record_max_size()
{
	return sizeof(record_t);
}
unsigned record_meta_size()
{
	return 2 * sizeof(uint32_t);
}

unsigned record_current_size(const record_clust_t* clust)
{
	BEDUG_ASSERT(clust->modbus1_count || clust->_1wire_count, "Record size must not be 0");
	if (!(clust->modbus1_count || clust->_1wire_count)) {
		return 0;
	}
	return record_meta_size() +
		   clust->modbus1_count * sizeof(modbus_sensor_t) +
		   clust->_1wire_count * sizeof(_1wire_sensor_t);
}

unsigned records_current_count(const record_clust_t* clust)
{
    if (record_current_size(clust)) {
        return 0;
    }

    uint32_t payload_clust = STORAGE_PAGE_PAYLOAD_SIZE - record_cluster_meta_size();
    return (record_current_size(clust) > payload_clust) ? 1 : (payload_clust / record_current_size(clust));
}

unsigned record_cluster_meta_size()
{
	return sizeof(record_clust_t) - sizeof(record_t);
}

unsigned record_cluster_size(const record_clust_t* clust)
{
	return record_cluster_meta_size() + records_current_count(clust) * record_current_size(clust);
}

bool record_cluster_validate(const record_clust_t* clust)
{
    if (clust->dv_type != settings.dv_type) {
        return false;
    }

    if (clust->vr_id != RECORD_CLUST_VERSION) {
        return false;
    }

    if (!record_current_size(clust) || record_current_size(clust) > record_max_size()) {
    	return false;
    }

    for (unsigned i = 0; i < records_current_count(clust); i++) {
    	if (get_record_modbus1_sensor(clust, 0, i)->ID) {
    		return true;
    	}
    	if (get_record_1wire_sensor(clust, 0, i)->ADDR) {
    		return true;
    	}
    }

    return false;
}

bool record_cluster_validate_size(const record_clust_t* source, const record_clust_t* target)
{
	return target->modbus1_count && target->_1wire_count &&
		   source->modbus1_count == target->modbus1_count &&
           source->_1wire_count == target->_1wire_count;
}

void record_create(record_t* record)
{
	record->time = clock_get_timestamp();
}

void record_cluster_create(record_clust_t* clust)
{
	memset((void*)&clust, 0, sizeof(clust));

	clust->dv_type       = settings.dv_type;
	clust->vr_id         = RECORD_CLUST_VERSION;
	clust->modbus1_count = settings_modbus1_count();
	clust->_1wire_count  = settings_1wire_count();
}

void record_cluster_show(const record_clust_t* clust)
{
#if RECORD_CLUST_BEDUG
	RTC_DateTypeDef date = {0};
	clock_get_rtc_date(&date);
	RTC_TimeTypeDef time = {0};
	clock_get_rtc_time(&time);

	printPretty("               %02u-%02u-20%02u\n", date.Date, date.Month, date.Year);
	printPretty("                %02u:%02u:%02u\n", time.Hours, time.Minutes, time.Seconds);
	printPretty("##############RECORD CLUST###############\n");
	printPretty("Device type: %u\n", clust->dv_type);
	printPretty("Record version v%02u\n", clust->vr_id);
	printPretty("MODBUS1 sensors count: %u\n", clust->modbus1_count);
	printPretty("1WIRE sensors count: %u\n", clust->_1wire_count);
    printPretty("INDEX   RCRDID    TIME       SENSID             VALUE\n");
    unsigned counter = 0;
	for (uint8_t i = 0; i < records_current_count(clust); i++) {
		record_t* record = get_record_by_index(clust, i);
		if (!record->id) {
			break;
		}
	    printPretty("%03u     %09lu %010lu ", i, record->id, record->time);
	    for (uint8_t j = 0; j < record_modbus1_sensors_count(clust); j++) {
	    	modbus_sensor_t* sensPtr = &(record->mb1_sens[j]);
	    	if (j == 0) {
	    		gprint("%03u            %u\n", sensPtr->ID, sensPtr->value);
	    	} else {
	    		printPretty("                             %03u            %u\n", sensPtr->ID, sensPtr->value);
	    	}
	    	counter++;
	    }
	    for (uint8_t j = 0; j < record_1wire_sensors_count(clust); j++) {
	    	_1wire_sensor_t* sensPtr = &(record->ow_sens[j]);
	    	if (j == 0) {
	    		gprint("0x%08X%08X %u\n", (unsigned)(sensPtr->ADDR >> 32), (unsigned)(sensPtr->ADDR), sensPtr->value);
	    	} else {
	    		printPretty("                             0x%08X%08X %u\n", (unsigned)(sensPtr->ADDR >> 32), (unsigned)(sensPtr->ADDR), sensPtr->value);
	    	}
	    	counter++;
	    }
	}
	if (!counter) {
        printPretty("------------------EMPTY------------------\n");
	}
	printPretty("##############RECORD CLUST###############\n");
#endif
}

void record_show(const record_clust_t* clust, const unsigned index)
{
	(void)clust;
	(void)index;
#if RECORD_BEDUG
	record_t* record = get_record_by_index(clust, index);
    printPretty("##########RECORD##########\n")
    printPretty("Record ID: %lu\n", record->id);
    printPretty("Record time: %lu\n", record->time);;
    printPretty("----------MODBUS1---------\n");
    printPretty("INDEX ID  VALUE\n");
    unsigned count = 0;
    while (count < clust->modbus1_count) {
    	modbus_sensor_t* sensor = get_record_modbus1_sensor(clust, index, count);
    	if (sensor->ID) {
    		printPretty("%03u   %03u        %u\n", count, sensor->ID, sensor->value);
    	}
    	count++;
    }
    if (!count) {
        printPretty("----------EMPTY-----------\n");
    }
    printPretty("-----------1WIRE----------\n");
    printPretty("INDEX ID       VALUE\n");
    count = 0;
    while (count < clust->_1wire_count) {
    	_1wire_sensor_t* sensor = get_record_1wire_sensor(clust, index, count);
    	if (sensor->ADDR) {
            printPretty("%03u   0x%08X%08X %u\n", count, (unsigned)(sensor->ADDR >> 32), (unsigned)(sensor->ADDR), sensor->value);
    	}
    	count++;
    }
    if (!count) {
        printPretty("----------EMPTY-----------\n");
    }
    printPretty("##########RECORD##########\n");
#endif
}

record_t* get_record_by_index(const record_clust_t* clust, const unsigned index)
{
	BEDUG_ASSERT(record_current_size(clust) > 0, "Record size must not be 0");
	BEDUG_ASSERT(index * record_current_size(clust) < record_max_size(), "Record index is out of range");
	return (record_t*)&(clust->records[index * record_current_size(clust)]);
}

unsigned record_modbus1_sensors_count(const record_clust_t* clust)
{
	return clust->modbus1_count;
}
modbus_sensor_t* get_record_modbus1_sensor(const record_clust_t* clust, const unsigned record_index, const unsigned sensor_index)
{
	BEDUG_ASSERT(clust->modbus1_count > 0, "Record sensors count must not be 0");
	BEDUG_ASSERT(sensor_index < clust->modbus1_count, "Record sensor index is out of range");
	return &(get_record_by_index(clust, record_index)->mb1_sens[sensor_index]);
}

unsigned record_1wire_sensors_count(const record_clust_t* clust)
{
	return clust->_1wire_count;
}
_1wire_sensor_t* get_record_1wire_sensor(const record_clust_t* clust, const unsigned record_index, const unsigned sensor_index)
{
	BEDUG_ASSERT(clust->_1wire_count > 0, "Record sensors count must not be 0");
	BEDUG_ASSERT(sensor_index < clust->_1wire_count, "Record sensor index is out of range");
	return &(get_record_by_index(clust, record_index)->ow_sens[sensor_index]);
}
