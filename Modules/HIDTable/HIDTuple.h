/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_HID_TUPLE_H_
#define _USB_HID_TUPLE_H_


#include <memory>

#include "bmacro.h"
#include "variables.h"


struct HIDTupleBase {};

template<class type_t, class getter_f = void>
struct HIDTuple : HIDTupleBase
{
    static_assert(!std::is_same<getter_f, void>::value, "Tuple getter functor must be non void");

    type_t* target(unsigned index = 0)
    {
        type_t* value = getter_f{}();
        BEDUG_ASSERT(value, "Value must not be null");
        if (!value) {
        	return nullptr;
        }
        return value + index;
    }

    type_t deserialize(const uint8_t* src)
    {
        BEDUG_ASSERT(src, "Source must not be null");
        if (!src) {
        	return 0;
        }
        return utl::deserialize<type_t>(src)[0];
    }

    std::shared_ptr<uint8_t[]> serialize(unsigned index = 0)
    {
        BEDUG_ASSERT(target(), "Target must not be null");
        if (!target()) {
        	return nullptr;
        }
        return utl::serialize<type_t>(target(index));
    }
};


#endif
