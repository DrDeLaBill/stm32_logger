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

    bool is_tartget(uint8_t* const target)
    {
        return target == reinterpret_cast<void*>(m_source);
    }

    uint8_t idx(const uint8_t index = 0)
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
            return;
        }
        memcpy(&m_source[idx(index) * item_size()], src, item_size());
    }

    void get(uint8_t* const dst, const uint8_t index = 0)
    {
        BEDUG_ASSERT(index <= length(), "Index is out of range");
        BEDUG_ASSERT(dst, "Destination must not be NULL");
        if (!dst || index >= length()) {
            return;
        }
        uint8_t tmp_index = idx(index);
        if (tmp_index < length()) {
            memcpy(dst, &m_source[tmp_index * item_size()], item_size());
        } else {
            memcpy(dst, EMPTY_DATA, item_size());
        }
    }
};


#endif
