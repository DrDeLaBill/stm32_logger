/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef HIDCONTROLLER_H
#define HIDCONTROLLER_H


#include <unordered_map>

#include "hid_defs.h"
#include "HIDTable.h"
#include "HIDTuple.h"


static const char REPORT_PREFIX[] = "LOG";


template<class Table>
struct HIDController
{
private:
    static_assert(std::is_base_of<HIDTableBase, Table>::value, "Template class must be HIDTable");

    using tuple_p = typename Table::tuple_p;
    using tuple_v = typename Table::tuple_v;

    using tuple_t = std::unordered_map<
        uint16_t,
        tuple_v
    >;

    template<class... TuplePacks>
    void set_table(utl::simple_list_t<TuplePacks...>)
    {
        (set_tuple(utl::getType<TuplePacks>{}), ...);
    }

    template<class TuplePack>
    void set_tuple(utl::getType<TuplePack> tuplePack)
    {
        using tuple_t = typename decltype(tuplePack)::TYPE;

        static uint16_t key = FIRST_KEY;

        characteristics.insert({key++, tuple_t{}});
    }

    void read()
    {
        uint16_t key = FIRST_KEY;
        auto it = characteristics.begin();
        while((it = characteristics.find(key++)) != characteristics.end()) {
            // TODO: read characteristics from USB
        }
    }

public:
    static constexpr uint16_t FIRST_KEY = 1;

    tuple_t characteristics;

    HIDController()
    {
        set_table(tuple_p{});
    }

    void setValue(const uint16_t key, const uint8_t* value, const uint8_t index = 0)
    {
        auto it = characteristics.find(key);
        if (it == characteristics.end()) {
        	BEDUG_ASSERT(false, "HID table not found error");
        	return;
        }

        auto lambda = [&] (auto& tuple) {
            tuple.target()[index] = tuple.deserialize(value);
        };

        std::visit(lambda, it->second);
    }

};


#endif // HIDCONTROLLER_H
