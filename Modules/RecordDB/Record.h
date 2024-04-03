/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _RECORD_H_
#define _RECORD_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "main.h"


#define RECORD_CLUST_BEDUG   (false)
#define RECORD_BEDUG         (false)

#define RECORD_ENABLE_CACHE  (true)
#define RECORD_CACHED_COUNT  (8)

#define RECORD_CLUST_VERSION (5)


typedef enum _RecordStatus {
    RECORD_OK = 0,
    RECORD_ERROR,
    RECORD_NO_LOG
} RecordStatus;


typedef struct __attribute__((packed)) _modbus_sensor_t {
    uint8_t  ID;
    uint16_t value;
} modbus_sensor_t;

typedef struct __attribute__((packed)) __1wire_sensor_t {
	uint64_t ADDR;
	uint16_t value;
} _1wire_sensor_t;


#define RECORD_SENSORS_MAX_SIZE (MODBUS_SENS_COUNT * (sizeof(modbus_sensor_t) + sizeof(_1wire_sensor_t)))

typedef struct __attribute__((packed)) _reocrd_t {
    uint32_t id;                                 // Record ID
    uint32_t time;                               // Record time
    uint8_t  sensors[RECORD_SENSORS_MAX_SIZE];   // Record sensors
} record_t;


typedef struct __attribute__((packed)) _record_clust_t {
	// Device type
    uint8_t  dv_type;
    // Record version
    uint8_t  vr_id;
    // Record MODBUS 1 sensors count
    uint8_t  modbus1_count;
    // Record 1WIRE sensors count
    uint8_t  _1wire_count;
    // Buffer for record/s
    uint8_t  records[sizeof(record_t)];
} record_clust_t;

#if RECORD_ENABLE_CACHE

typedef struct _record_cache_t {
	record_clust_t cluster;
    uint8_t        modbus1_count; // TODO: ?
    uint8_t        _1wire_count; // TODO: ?
	uint32_t       address;
} record_cache_t;

#endif


unsigned  record_max_size();
unsigned  record_meta_size();
unsigned  record_current_size(const record_clust_t* clust);
unsigned  records_current_count(const record_clust_t* clust);
unsigned  record_cluster_meta_size();
unsigned  record_cluster_size(const record_clust_t* clust);
bool      record_cluster_validate(const record_clust_t* clust);
bool      record_cluster_validate_size(const record_clust_t* source, const record_clust_t* target);
void      record_create(record_t* record);
void      record_cluster_create(record_clust_t* clust);
void      record_cluster_show(const record_clust_t* clust);
void      record_show(const record_clust_t* clust, const unsigned index);
record_t* get_record_by_index(const record_clust_t* clust, const unsigned index);

unsigned         record_modbus1_sensors_count(const record_clust_t* clust);
modbus_sensor_t* get_record_modbus1_sensor(record_t* record, const unsigned sensor_index);
void set_record_modbus1_measure(
	record_t*              record,
	const uint8_t          index,
	const modbus_sensor_t* measure
);

unsigned         record_1wire_sensors_count(const record_clust_t* clust);
_1wire_sensor_t* get_record_1wire_sensor(record_t* record, const uint8_t modbus1_count, const unsigned sensor_index);
void set_record_1wire_measure(
	record_t*              record,
	const uint8_t          modbus1_count,
	const uint8_t          index,
	const _1wire_sensor_t* measure
);

#ifdef __cplusplus
}
#endif


#endif
