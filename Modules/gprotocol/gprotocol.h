/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _GTRANSFER_H_
#define _GTRANSFER_H_


#include <unordered_map>

#include "hal_defs.h"

#include "glog.h"
#include "gutils.h"
#include "gtuple.h"
#include "greport.h"

#ifdef USE_HAL_DRIVER
#   include "main.h"
#   include "usbd_cdc_if.h"
#elif defined(ESP32)
	// Include Витали
#else
#   error Please check your platform
#endif


#ifndef GP_KEY_NAME
#   define GP_KEY_NAME(VAR) gprotocol::str_hash((char*)__STR_DEF2__(VAR))
#endif

#ifndef GP_KEY_STR
#   define GP_KEY_STR(STR) gprotocol::str_hash((char*)STR)
#endif


struct gprotocol
{
private:
	static constexpr char TAG[] = "GPTL";

	using type_t = uint64_t;

public:
	static constexpr uint32_t str_hash(char* const data)
	{
		uint32_t hash = 0;

		for(unsigned i = 0; i < strlen(data); i++) {
			hash += data[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}

		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);

		return hash;
	}

private:
	std::unordered_map<uint32_t, gtuple>& table;

	void send_report(pack_t* const report)
	{
#ifdef USE_HAL_DRIVER
		CDC_Transmit_FS((uint8_t*)report, sizeof(pack_t));
#endif
	}

	uint8_t index(const uint32_t key, const uint8_t index = 0)
	{
		auto it = table.find(key);
		if (it == table.end()) {
            BEDUG_ASSERT(false, "Table not found error");
            return 0;
		}
		return index;
	}

    static type_t deserialize(const uint8_t* src, const unsigned size)
    {
    	type_t value = std::numeric_limits<type_t>::min();

        if (!src) {
            BEDUG_ASSERT(false, "The source must not be null");
        	return value;
        }

		for (unsigned i = 0; i < size; i++) {
			uint8_t tmp = src[i];
			value <<= BITS_IN_BYTE;
			value |= tmp;
		}

		return value;
    }

    static uint8_t* serialize(const type_t value, const unsigned size)
    {
        static uint8_t serialized[sizeof(type_t)];
    	memset(serialized, 0, sizeof(serialized));

    	type_t tmp = value;
		for (unsigned i = size; i > 0; i--) {
			serialized[i - 1] = static_cast<uint8_t>(tmp & 0xFF);
			tmp >>= BITS_IN_BYTE;
		}

		return serialized;
    }

    void set_from_serialized(const uint32_t key, uint8_t* const value, const uint8_t index = 0)
    {
    	auto it = table.find(key);
    	if (it == table.end()) {
        	BEDUG_ASSERT(false, "Table not found error");
        	return;
    	}

		type_t tmp = deserialize(value, it->second.item_size());
		it->second.set(reinterpret_cast<uint8_t*>(&tmp), index);
    }

    void get_serialized(const uint32_t key, uint8_t* dst, const uint8_t index = 0)
    {
    	auto it = table.find(key);
    	if (it == table.end()) {
        	BEDUG_ASSERT(false, "Table not found error");
        	return;
    	}

		type_t tmp = 0;
		it->second.get(reinterpret_cast<uint8_t*>(&tmp), index);
		memcpy(dst, serialize(tmp, it->second.item_size()), it->second.item_size());
    }


public:
	gprotocol(std::unordered_map<uint32_t, gtuple>& table): table(table) {}

	void slave_recieve(pack_t* request)
	{
		if (request->crc != pack_crc(request)) {
			return;
		}

    	printTagLog(TAG, "Request:");
    	pack_show(request);

    	pack_t response = {};
    	response.key    = 0;
    	response.index  = request->index;

    	if (request->key == PACK_GETTER_KEY) {
    		response.key   = static_cast<uint32_t>(deserialize(request->data, sizeof(response.key)));
    		response.index = index(response.key, request->index);
    	} else {
    		response.key = request->key;
    		set_from_serialized(request->key, request->data, request->index);
    	}

    	get_serialized(response.key, response.data, response.index);

    	response.crc = pack_crc(&response);

    	send_report(&response);

    	printTagLog(TAG, "Response:");
    	pack_show(&response);
	}

	void master_send(const bool send, const uint32_t key, const uint8_t index = 0)
	{
		pack_t request = {};
		request.key = PACK_GETTER_KEY;
		request.index = index;

		if (send) {
			request.key = key;
			get_serialized(key, request.data, index);
		} else {
			memcpy(request.data, serialize(static_cast<type_t>(key), sizeof(key)), sizeof(key));
		}

		request.crc = pack_crc(&request);

    	send_report(&request);
    	printTagLog(TAG, "Request:");
    	pack_show(&request);
	}

	bool master_recieve(pack_t* response)
	{
		if (response->crc != pack_crc(response)) {
			return false;
		}

		set_from_serialized(response->key, response->data, response->index);

    	printTagLog(TAG, "Response:");
    	pack_show(response);

    	return true;
	}

};


#endif
