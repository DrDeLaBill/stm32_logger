/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "StorageAT.h"
#include "RecordType.h"


class Record
{
public:
    record_t record; // TODO: change variable type to ptr

    Record(uint32_t recordId, uint16_t sensCount = 0);

    sensor_t& operator[](const unsigned i);

    RecordStatus save();
    RecordStatus load();
    RecordStatus loadNext();

    void show();
    unsigned size();
    unsigned count();

    void set(uint8_t ID, uint16_t value);
    uint16_t get(uint8_t ID);

private:
    static constexpr char TAG[] = "RCR";
    static constexpr char PREFIX[] = "RCR";

    uint32_t m_recordId;
    uint16_t m_sensCount;
    uint8_t m_counter;
};
