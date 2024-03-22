/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef HIDCONTROLLER_H
#define HIDCONTROLLER_H


#include <variant>
#include <cstring>
#include <unordered_map>

#include "utils.h"
#include "com_defs.h"
#include "COMTable.h"
#include "COMTuple.h"

#ifdef USE_HAL_DRIVER
#	include "log.h"
#	include "bmacro.h"
#else
#	include "app_exception.h"
#endif


template<class Table>
struct COMController
{
public:
    static_assert(std::is_base_of<COMTableBase, Table>::value, "Template class must be HIDTable");

    static constexpr unsigned count()
    {
        return Table::count();
    }

private:
    using tuple_p = typename Table::tuple_p;
    using tuple_v = typename Table::tuple_v;

    using tuple_t = std::unordered_map<
        uint16_t,
        tuple_v
    >;

    uint16_t currID;

#if defined(USE_HAL_DRIVER) && COM_TABLE_BEDUG
    void details(const uint16_t key, const uint8_t index)
    {
        gprint("Details: key = %u; index = %u; max key = %u\n", key, index, count() - 1);
    }
#endif

    template<class... TuplePacks>
    void set_table(utl::simple_list_t<TuplePacks...>)
    {
        (set_tuple(utl::getType<TuplePacks>{}), ...);
        currID--;
    }

    template<class TuplePack>
    void set_tuple(utl::getType<TuplePack> tuplePack)
    {
        using tuple_t = typename decltype(tuplePack)::TYPE;

        characteristics.insert({currID, tuple_t{}});

#ifndef USE_HAL_DRIVER
        auto it = characteristics.find(currID);
        auto lambda = [&] (const auto& tuple) {
            tuple.setID(currID);
        };
        std::visit(lambda, it->second);
#endif

        currID++;
    }

public:
    tuple_t characteristics;

    COMController(const uint16_t startKey = COM_FIRST_KEY)
    {
        currID = startKey;
        set_table(tuple_p{});
    }

    void setValue(const uint16_t key, const uint8_t* value, const uint8_t index = 0)
    {
        auto it = characteristics.find(key);
        if (it == characteristics.end()) {
#ifdef USE_HAL_DRIVER
#	if COM_TABLE_BEDUG
        	BEDUG_ASSERT(false, "HID table not found error");
        	details(key, index);
#	endif
        	return;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }

        auto lambda = [&] (auto& tuple) {
            tuple.set(tuple.deserialize(value), index);
        };

        std::visit(lambda, it->second);
    }

    void getValue(const uint16_t key, uint8_t* dst, const uint8_t index = 0)
    {
    	auto it = characteristics.find(key);
        if (it == characteristics.end()) {
#ifdef USE_HAL_DRIVER
#	if COM_TABLE_BEDUG
        	BEDUG_ASSERT(false, "HID table not found error");
        	details(key, index);
#	endif
        	return;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }

        auto lambda = [&] (auto& tuple) {
            memcpy(dst, tuple.serialize(index).get(), __min(sizeof(uint32_t), tuple.size()));
        };

        std::visit(lambda, it->second);
    }

#ifndef USE_HAL_DRIVER
    bool isUpdated(const uint16_t key, const unsigned index = 0)
    {
        auto it = characteristics.find(key);
        if (it == characteristics.end()) {
            throw new exceptions::TemplateErrorException();
        }

        bool result = false;
        auto lambda = [&] (auto& tuple) {
            result = tuple.isUpdated(index);
        };
        std::visit(lambda, it->second);

        return result;
    }

    void resetUpdated(const uint16_t key, const unsigned index = 0)
    {
        auto it = characteristics.find(key);
        if (it == characteristics.end()) {
            throw new exceptions::TemplateErrorException();
        }

        auto lambda = [&] (auto& tuple) {
            tuple.resetUpdated(index);
        };
        std::visit(lambda, it->second);
    }
#else
    unsigned getIndex(const uint16_t key, const uint8_t index = 0)
    {
    	auto it = characteristics.find(key);
    	if (it == characteristics.end()) {
#	if COM_TABLE_BEDUG
        	BEDUG_ASSERT(false, "HID table not found error");
        	details(key, index);
#	endif
        	return 0;
    	}

        unsigned result = 0;
        auto lambda = [&] (auto& tuple) {
        	result = tuple.index(index);
        };
        std::visit(lambda, it->second);

        return result;
    }
#endif

    constexpr unsigned characteristicLength(uint16_t characteristic_id)
    {
        auto it = characteristics.find(characteristic_id);
        if (it == characteristics.end()) {
#ifdef USE_HAL_DRIVER
#	if COM_TABLE_BEDUG
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
