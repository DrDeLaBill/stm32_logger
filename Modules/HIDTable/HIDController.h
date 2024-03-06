/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef HIDCONTROLLER_H
#define HIDCONTROLLER_H


#include <variant>
#include <cstring>
#include <unordered_map>

#include "log.h"
#include "utils.h"
#include "hid_defs.h"
#include "HIDTable.h"
#include "HIDTuple.h"

#if defined(USE_HAL_DRIVER) && defined(DEBUG)
#	include "bmacro.h"
#else
#	include "app_exception.h"
#endif


template<class Table>
struct HIDController
{
private:
    uint16_t max_key;

    static_assert(std::is_base_of<HIDTableBase, Table>::value, "Template class must be HIDTable");

    using tuple_p = typename Table::tuple_p;
    using tuple_v = typename Table::tuple_v;

    using tuple_t = std::unordered_map<
        uint16_t,
        tuple_v
    >;

    void details(const uint16_t key, const uint8_t index)
    {
#if defined(USE_HAL_DRIVER) && HID_TABLE_BEDUG
        gprint("Details: key = %u; index = %u; max key = %u\n", key, index, maxKey());
#endif
    }

    template<class... TuplePacks>
    void set_table(utl::simple_list_t<TuplePacks...>)
    {
        (set_tuple(utl::getType<TuplePacks>{}), ...);
        max_key--;
    }

    template<class TuplePack>
    void set_tuple(utl::getType<TuplePack> tuplePack)
    {
        using tuple_t = typename decltype(tuplePack)::TYPE;

        characteristics.insert({max_key++, tuple_t{}});
    }

public:
    tuple_t characteristics;

    HIDController()
    {
        max_key = HID_FIRST_KEY;
        set_table(tuple_p{});
    }

    void setValue(const uint16_t key, const uint8_t* value, const uint8_t index = 0)
    {
        auto it = characteristics.find(key);
        if (it == characteristics.end()) {
#if defined(USE_HAL_DRIVER)
#	if HID_TABLE_BEDUG
        	BEDUG_ASSERT(false, "HID table not found error");
        	details(key, index);
#	endif
        	return;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }

        auto lambda = [&] (auto& tuple) {
            tuple.set(tuple.deserialize(value), tuple.index(index));
        };

        std::visit(lambda, it->second);
    }

    void getValue(const uint16_t key, uint8_t* dst, const uint8_t index = 0)
    {
    	auto it = characteristics.find(key);
        if (it == characteristics.end()) {
#if defined(USE_HAL_DRIVER)
#	if HID_TABLE_BEDUG
        	BEDUG_ASSERT(false, "HID table not found error");
        	details(key, index);
#	endif
        	return;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }

        auto lambda = [&] (auto& tuple) {
            memcpy(dst, tuple.serialize(tuple.index(index)).get(), __min(sizeof(uint32_t), tuple.size()));
        };

        std::visit(lambda, it->second);
    }

    unsigned getIndex(const uint16_t key, const uint8_t index = 0)
    {
    	auto it = characteristics.find(key);
        if (it == characteristics.end()) {
#if defined(USE_HAL_DRIVER) && defined(DEBUG)
#	if HID_TABLE_BEDUG
        	BEDUG_ASSERT(false, "HID table not found error");
        	details(key, index);
#	endif
        	return 0;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }

        unsigned result = 0;
        auto lambda = [&] (auto& tuple) {
        	result = tuple.index(index);
        };
        std::visit(lambda, it->second);

        return result;
    }

    constexpr unsigned maxKey()
    {
        return max_key;
    }

    constexpr unsigned characteristicLength(uint16_t characteristic_id)
    {
        auto it = characteristics.find(characteristic_id);
        if (it == characteristics.end()) {
#if defined(USE_HAL_DRIVER) && defined(DEBUG)
#	if HID_TABLE_BEDUG
            BEDUG_ASSERT(false, "HID table not found error");
        	details(characteristic_id, 0);
#	endif
            return 0;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }

        unsigned result = 0;
        auto lambda = [&] (const auto& tuple) {
            result = tuple.length();
        };

        std::visit(lambda, it->second);

        return result;
    }

};


#endif // HIDCONTROLLER_H
