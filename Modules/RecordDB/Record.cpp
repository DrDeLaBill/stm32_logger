/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Record.h"

#include <cstring>
#include <stdint.h>

#include "log.h"
#include "soul.h"
#include "clock.h"
#include "settings.h"
#include "StorageAT.h"
#include "RecordType.h"
#include "RecordClust.h"
#include "StorageDriver.h"


Record::Record(uint32_t recordId, uint16_t sensCount):
	m_recordId(recordId), m_sensCount(sensCount), m_counter(0)
{
//	BEDUG_ASSERT(sensCount > 0, "Record sensors count must not be 0"); // TODO: remove
    memset(reinterpret_cast<size_t*>(&record), 0, sizeof(record));
}

sensor_t& Record::operator[](const unsigned i)
{
#if RECORD_BEDUG
	BEDUG_ASSERT((i < this->count()), "Record sensors index is out of range"); // TODO: is check right?
#endif
    return this->record.sens[i];
}

RecordStatus Record::load()
{
    RecordClust clust(m_recordId, this->size());

    RecordStatus recordStatus = clust.load(true);
    if (recordStatus != RECORD_OK) {
        return recordStatus;
    }

    bool recordFound = false;
    unsigned id;
    for (unsigned i = 0; i < clust.records_count(); i++) {
        if (clust[i].id == this->m_recordId) {
            recordFound = true;
            id = i;
            break;
        }
    }
    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(TAG, "Record not found");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(reinterpret_cast<void*>(&(this->record)), reinterpret_cast<void*>(&(clust[id])), this->size());
    m_sensCount = getSensorsCountBySize(clust.record_size());
    m_recordId  = this->record.id;

#if RECORD_BEDUG
    printTagLog(TAG, "Record loaded (cluster index=%u)", id);
    this->show();
#endif

    return RECORD_OK;
}

RecordStatus Record::loadNext()
{
    RecordClust clust(m_recordId + 1, this->size());

    RecordStatus recordStatus = clust.load(false);
    if (recordStatus != RECORD_OK) {
        return recordStatus;
    }

    bool recordFound = false;
    unsigned idx;
    uint32_t curId = 0xFFFFFFFF;
    for (unsigned i = 0; i < clust.records_count(); i++) {
        if (clust[i].id > m_recordId && curId > clust[i].id) {
            curId = clust[i].id;
            recordFound = true;
            idx = i;
            break;
        }
    }

    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(TAG, "Record not found");
#endif
        return RECORD_NO_LOG;
    }

    m_sensCount = getSensorsCountBySize(clust.record_size());
    m_recordId  = curId;
    memcpy(
        reinterpret_cast<void*>(&(this->record)),
        reinterpret_cast<void*>(&(clust[idx])),
        this->size()
    );

#if RECORD_BEDUG
    printTagLog(TAG, "Next record loaded (cluster index=%u)", idx);
    this->show();
#endif

    return RECORD_OK;
}

#if RECORD_ENABLE_CACHE

RecordStatus Record::updateCache(uint32_t cacheAfterId)
{
	return RecordClust::updateCache(cacheAfterId);
}

#endif

RecordStatus Record::getLastTime(uint32_t* time)
{
	return RecordClust::getLastTime(time);
}

RecordStatus Record::getMinId(uint32_t* id)
{
	return RecordClust::getMinId(id);
}

RecordStatus Record::getMaxId(uint32_t* id)
{
	return RecordClust::getMaxId(id);
}

RecordStatus Record::save()
{
	if (!this->count()) {
		return RECORD_ERROR;
	}

    RecordClust clust(0, this->size());
    RecordStatus recordStatus = RECORD_OK;

    record.time = clock_get_timestamp();
    recordStatus = clust.save(&record, this->size());

    if (recordStatus == RECORD_OK) {
    	set_status(NEED_LOAD_MIN_RECORD);
    	set_status(NEED_LOAD_MAX_RECORD);
#ifdef RECORD_BEDUG
    	clust.show();
        this->show();
    } else {
        printTagLog(TAG, "New record was not saved");
#endif
    }

    return recordStatus;
}

void Record::show()
{
#if RECORD_BEDUG
    printPretty("##########RECORD##########\n");
    printPretty("Record ID: %lu\n", record.id);
    printPretty("Record time: %lu\n", record.time);
    printPretty("INDEX\tID\tVALUE\n");
    for (uint8_t i = 0; i < this->count(); i++) {
        printPretty("%03u\t%03u\t%u\n", i, record.sens[i].ID, record.sens[i].value);
    }
    if (!this->count()) {
        printPretty("----------EMPTY-----------\n");
    }
    printPretty("##########RECORD##########\n");
#endif
}

unsigned Record::size()
{
    return RECORD_META_SIZE + m_sensCount * sizeof(_sensor_t);
}

unsigned Record::count()
{
	BEDUG_ASSERT(m_sensCount > 0, "Sensors count must not be 0");
    return m_sensCount;
}

void Record::set(uint8_t ID, uint16_t value)
{
	BEDUG_ASSERT(ID && ID <= MODBUS_SENS_COUNT, "Sensor index is out of range");

	for (uint8_t i = 0; i < this->count(); i++) {
		if (record.sens[i].ID == ID) {
			record.sens[i].value = value;
			return;
		}
	}

	record.sens[m_counter].ID    = ID;
	record.sens[m_counter].value = value;
	m_counter++;
}

uint16_t Record::get(uint8_t ID)
{
	BEDUG_ASSERT(ID < this->count(), "Sensor index is out of range"); // TODO: is check right?

	for (uint8_t i = 0; i < this->count(); i++) {
		if (record.sens[i].ID == ID) {
			return record.sens[i].value;
		}
	}

#if RECORD_BEDUG
	printTagLog(TAG, "Sensor with ID=%u not found", ID);
#endif
	return 0;
}

uint32_t Record::getSensorsCountBySize(uint32_t recordSize)
{

#if RECORD_BEDUG
    BEDUG_ASSERT((recordSize > RECORD_META_SIZE), "Record size must be large than RECORD_META_SIZE");
#endif
    if (recordSize == 0) {
        return 0;
    }
    return (recordSize - RECORD_META_SIZE) / sizeof(sensor_t);
}
