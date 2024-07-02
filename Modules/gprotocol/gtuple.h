/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _GTUPLE_H_
#define _GTUPLE_H_


#include <limits>
#include <memory>
#include <cstddef>
#include <cstdint>

#include "glog.h"

#include "variables.h"


struct gtuple
{
private:
    static constexpr char TAG[] = "GTPL";

    uint8_t* const source;
    const unsigned SIZE;
    const unsigned LENGTH;

public:
    gtuple(uint8_t* const source, unsigned size, unsigned length = 1):
    	source(source), SIZE(size), LENGTH(length)
    {
    	BEDUG_ASSERT(size, "Size must not be 0");
    	BEDUG_ASSERT(length, "Length must not be 0");
    }

    unsigned length()
    {
    	return LENGTH;
    }

    unsigned item_size()
    {
        return SIZE;
    }

    unsigned full_size()
    {
        return item_size() * length();
    }

    bool is_tartget(uint8_t* const tartget)
    {
    	return tartget == reinterpret_cast<void*>(source);
    }

    void details(const uint8_t index = 0)
    {
        printTagLog(TAG, "Details: pointer=%hhn index = %u; size = %d; length = %u\n", (int8_t*)source, index, item_size(), length());
    }

    void set(uint8_t* const src, const uint8_t index = 0)
    {
    	BEDUG_ASSERT(index < full_size(), "Index is out of range");
    	BEDUG_ASSERT(src, "Source must not be NULL");
    	if (!src || index >= full_size()) {
    		return;
    	}
    	memcpy(&source[index * item_size()], src, item_size());
    }

    void get(uint8_t* const dst, const uint8_t index = 0)
    {
    	BEDUG_ASSERT(index < length(), "Index is out of range");
    	BEDUG_ASSERT(dst, "Destination must not be NULL");
    	if (!dst) {
    		return;
    	}
    	uint8_t* tmp = &source[full_size() - 1];
    	if (index < full_size()) {
    		tmp = &source[index * item_size()];
    	}
    	memcpy(dst, tmp, item_size());
    }
};


#endif
