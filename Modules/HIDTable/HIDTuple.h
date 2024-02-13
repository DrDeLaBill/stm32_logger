/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_HID_TUPLE_H_
#define _USB_HID_TUPLE_H_


#include <memory>
#include <cstdint>

#ifdef USE_HAL_DRIVER
#   include "bmacro.h"
#else
#   include "app_exception.h"
#endif

#include "variables.h"


struct HIDTupleBase {};

template<class type_t, class getter_f = void, unsigned LENGTH = 1>
struct HIDTuple : HIDTupleBase
{
    static_assert(!std::is_same<getter_f, void>::value, "Tuple getter functor must be non void");
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
        gprint("Details: target pointer = %u; index = %u; size = %d; length = %u\n", reinterpret_cast<unsigned>(target()), index, sizeof(type_t), length());
#endif
    }

    type_t* target(unsigned index = 0)
    {
        if (index >= length()) {
#ifdef USE_HAL_DRIVER
            BEDUG_ASSERT(false, "The value index is large than the value length");
            details(index);
        	return nullptr;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        type_t* value = getter_f{}();
        if (!value) {
#ifdef USE_HAL_DRIVER
            BEDUG_ASSERT(false, "Value must not be null");
            details(index);
        	return nullptr;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        return value + index; // * sizeof(type_t);
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
    	type_t* value = target(index);
        if (!value || index >= length()) {
#ifdef USE_HAL_DRIVER
            BEDUG_ASSERT(false, "Target must not be null");
            details(index);
        	return nullptr;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        return utl::serialize<type_t>(value);
    }
};


#endif
