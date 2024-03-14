/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _HID_TABLE_WORKER_H_
#define _HID_TABLE_WORKER_H_


#include <cstdint>

#include "HIDController.h"
#include "TypeListService.h"


template<class Table, uint16_t START_ID>
struct HIDTableWorker
{
    static_assert(
        !utl::empty(typename utl::typelist_t<Table>::RESULT{}),
        "HIDTableWorker must not be empty"
    );

    static HIDController<Table> hid_table;

    static constexpr uint16_t minID()
    {
        return START_ID;
    }

    static constexpr uint16_t maxID()
    {
        return START_ID + HIDController<Table>::count() - 1;
    }
};

template<class Table, uint16_t START_ID>
HIDController<Table> HIDTableWorker<Table, START_ID>::hid_table(START_ID);


#endif
