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

	static constexpr uint8_t EMPTY_DATA[sizeof(uint64_t)] = {};

    const unsigned SIZE;
    const unsigned LENGTH;

    uint8_t* const m_source;
    uint8_t (*const m_at) (uint8_t);

    void details(uint8_t* const ptr, const uint8_t index = 0)
    {
    	(void)ptr;
    	(void)index;
    	printPretty("details: poiter=%p, index=%u\n", (void*)ptr, index);
    }

public:
    gtuple(
        uint8_t* const source,
        const unsigned size,
        const unsigned length = 1,
        uint8_t (*const at) (uint8_t) = nullptr
    ):
        SIZE(size),
        LENGTH(length),
        m_source(source),
		m_at(at)
    {
        BEDUG_ASSERT(size, "Size must not be 0");
        BEDUG_ASSERT(length, "Length must not be 0");
    }

    unsigned length() const
    {
        return LENGTH;
    }

    unsigned item_size() const
    {
        return SIZE;
    }

    unsigned full_size() const
    {
        return item_size() * length();
    }

    bool is_tartget(uint8_t* const target) const
    {
        return target == reinterpret_cast<void*>(m_source);
    }

    uint8_t index(const uint8_t index = 0) const
    {
    	uint8_t tmp_index = index;
    	if (m_at) {
    		tmp_index = m_at(index);
    	}
    	return tmp_index;
    }

    void set(uint8_t* const src, const uint8_t index = 0)
    {
        BEDUG_ASSERT(index <= length(), "Index is out of range");
        BEDUG_ASSERT(src, "Source must not be NULL");
        if (!src || index >= length()) {
        	if (!src || index > length()) {
        		details(src, index);
        	}
            return;
        }
        memcpy(&m_source[index * item_size()], src, item_size());
    }

    void get(uint8_t* const dst, const uint8_t index = 0)
    {
        BEDUG_ASSERT(index <= length(), "Index is out of range");
        BEDUG_ASSERT(dst, "Destination must not be NULL");
        if (!dst || index >= length()) {
        	if (!dst || index > length()) {
        		details(dst, index);
        	}
            return;
        }
        if (index < length()) {
            memcpy(dst, &m_source[index * item_size()], item_size());
        } else {
            memcpy(dst, EMPTY_DATA, item_size());
        }
    }
};


#endif
