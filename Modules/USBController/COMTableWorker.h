/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _HID_TABLE_WORKER_H_
#define _HID_TABLE_WORKER_H_


#include <cstdint>

#include "COMController.h"
#include "TypeListService.h"


template<class Table, uint16_t START_ID>
struct COMTableWorker
{
    static_assert(
        !utl::empty(typename utl::typelist_t<Table>::RESULT{}),
        "HIDTableWorker must not be empty"
    );

    static COMController<Table> hid_table;

    static constexpr uint16_t minID()
    {
        return START_ID;
    }

    static constexpr uint16_t maxID()
    {
        return START_ID + COMController<Table>::count() - 1;
    }
};

template<class Table, uint16_t START_ID>
COMController<Table> COMTableWorker<Table, START_ID>::hid_table(START_ID);


#endif
