/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include "main.h"


#define RECORD_BEDUG        (false)
#define RECORD_CLUST_BEDUG  (false)

#define RECORD_ENABLE_CACHE (true)
#define RECORD_CACHED_COUNT (3)


typedef enum _RecordStatus {
    RECORD_OK = 0,
    RECORD_ERROR,
    RECORD_NO_LOG
} RecordStatus;


typedef struct __attribute__((packed)) _sensor_t {
    uint8_t  ID;
    uint16_t value;
} sensor_t;

static const unsigned SENS_SIZE_MAX = sizeof(sensor_t) * MODBUS_SENS_COUNT;
typedef struct __attribute__((packed)) _reocrd_t {
    uint32_t id;                      // Record ID
    uint32_t time;                    // Record time
    sensor_t sens[MODBUS_SENS_COUNT]; // Record registers values
} record_t;

static const unsigned RECORD_META_SIZE = sizeof(record_t) - SENS_SIZE_MAX;
static const unsigned RECORD_SIZE_MAX  = sizeof(record_t);
