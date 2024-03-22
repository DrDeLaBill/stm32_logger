/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_HID_TUPLE_H_
#define _USB_HID_TUPLE_H_


#include <memory>
#include <cstdint>


#ifdef USE_HAL_DRIVER
#	include "com_defs.h"

#	include "log.h"
#   include "bmacro.h"
#   include "variables.h"
#else
#   include "app_exception.h"
#   include "variables.h"
#endif



struct COMTupleBase {};

template<class type_t, class callback_c = void, unsigned LENGTH = 1>
struct COMTuple : COMTupleBase
{
    static_assert(!std::is_same<callback_c, void>::value, "The tuple getter functor must be non void");
    static_assert(LENGTH > 0, "The length must not be 0");


#ifndef USE_HAL_DRIVER
    COMTuple()
    {
        if (callback_c::updated) {
            throw new exceptions::TemplateErrorException();
        }
        callback_c::updated = new bool[LENGTH];
    }
#endif

    constexpr unsigned size() const // TODO: find references
    {
        return sizeof(type_t);
    }

    constexpr unsigned length() const
    {
        return LENGTH;
    }

#if defined(USE_HAL_DRIVER) && COM_TABLE_BEDUG
    void details(const unsigned index = 0)
    {
        gprint("Details: index = %u; size = %d; length = %u\n", index, sizeof(type_t), length());
    }
#endif

    type_t deserialize(const uint8_t* src)
    {
        if (!src) {
#ifdef USE_HAL_DRIVER
#	if COM_TABLE_BEDUG
            BEDUG_ASSERT(false, "The source must not be null");
#	endif
        	return 0;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        return utl::deserialize<type_t>(src)[0];
    }

    std::shared_ptr<uint8_t[]> serialize(const unsigned index = 0)
    {
        type_t value = static_cast<type_t>(callback_c::get(index));
        if (index >= length()) {
#ifdef USE_HAL_DRIVER
#	if COM_TABLE_BEDUG
            BEDUG_ASSERT(false, "The target must not be null");
            details(index);
#	endif
        	return nullptr;
#else
            throw new exceptions::TemplateErrorException();
#endif
        }
        return utl::serialize<type_t>(&value);
    }

    void set(type_t value, const unsigned index = 0)
    {
    	if (value == callback_c::get(index)) {
    		return;
    	}
        callback_c::set(value, index);
    }

#ifndef USE_HAL_DRIVER

    bool isUpdated(const unsigned index = 0)
    {
        if (!callback_c::updated) {
            throw new exceptions::TemplateErrorException();
        }
        return callback_c::updated[index];
    }

    void resetUpdated(const unsigned index = 0)
    {
        if (!callback_c::updated) {
            throw new exceptions::TemplateErrorException();
        }
        callback_c::updated[index] = false;
    }

    void setID(const uint16_t ID) const
    {
        callback_c::ID = ID;
    }

#else

    unsigned index(const unsigned index = 0)
    {
        if (index > LENGTH - 1) {
#	if COM_TABLE_BEDUG
            BEDUG_ASSERT(false, "Target index is out of range");
        	details(index);
#	endif
            return 0;
        }
    	unsigned result = callback_c::index(index);
    	if (result >= length()) {
#	if COM_TABLE_BEDUG
            BEDUG_ASSERT(false, "Found index is out of range");
        	details(result);
#	endif
    		return length() - 1;
    	}
    	return result;
    }

#endif
};


#endif
