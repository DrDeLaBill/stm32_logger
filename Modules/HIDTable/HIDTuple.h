/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_HID_TUPLE_H_
#define _USB_HID_TUPLE_H_


#include <memory>
#include <cstdint>

#ifdef USE_HAL_DRIVER
#   include "bmacro.h"
#   include "variables.h"
#else
#   include "app_exception.h"
#   include "variables.h"
#endif



struct HIDTupleBase {};

template<class type_t, class callback_c = void, unsigned LENGTH = 1>
struct HIDTuple : HIDTupleBase
{
    static_assert(!std::is_same<callback_c, void>::value, "Tuple getter functor must be non void");
    static_assert(LENGTH > 0, "Length must not be 0");


    constexpr unsigned size() const // TODO: find references
    {
        return sizeof(type_t);
    }

    constexpr unsigned length() const
    {
        return LENGTH;
    }

    void details(unsigned index = 0)
    {
#ifdef USE_HAL_DRIVER
        gprint("Details: target pointer = %lu; index = %u; size = %d; length = %u\n", get(), index, sizeof(type_t), length());
#endif
    }

    type_t deserialize(const uint8_t* src)
    {
        if (!src) {
#ifdef USE_HAL_DRIVER
            BEDUG_ASSERT(false, "Source must not be null");
        	return 0;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        return utl::deserialize<type_t>(src)[0];
    }

    std::shared_ptr<uint8_t[]> serialize(unsigned index = 0)
    {
        type_t value = static_cast<type_t>(callback_c{}.get(index));
        if (index >= length()) {
#ifdef USE_HAL_DRIVER
            BEDUG_ASSERT(false, "Target must not be null");
            details(index);
        	return nullptr;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        return utl::serialize<type_t>(&value);
    }

    void set(type_t value, unsigned index = 0)
    {
        callback_c{}.set(value, index);
    }

    type_t get(unsigned index = 0)
    {
        return callback_c{}.get(index);
    }
};


#endif
