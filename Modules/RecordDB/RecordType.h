/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include "main.h"


#define RECORD_BEDUG (true)


typedef enum _RecordStatus {
    RECORD_OK = 0,
    RECORD_ERROR,
    RECORD_NO_LOG
} RecordStatus;


typedef struct __attribute__((packed)) _sensor_t {
    uint8_t  ID;
    uint16_t value;
} sensor_t;

static const unsigned SENS_SIZE_MAX = sizeof(uint16_t) * MODBUS_SENS_COUNT;
static const unsigned RECORD_META_SIZE = sizeof(uint32_t) * 2;
static const unsigned RECORD_SIZE_MAX = RECORD_META_SIZE + SENS_SIZE_MAX;
typedef struct __attribute__((packed)) _reocrd_t {
    uint32_t id;                  // Record ID
    uint32_t time;                // Record time
    sensor_t sens[SENS_SIZE_MAX]; // Record registers values
} record_t;
