/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _GTRANSFER_H_
#define _GTRANSFER_H_


#include <string>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

#include "hal_defs.h"

#include "glog.h"
#include "gutils.h"
#include "gtuple.h"
#include "greport.h"

#ifdef USE_HAL_DRIVER
#   include "main.h"
#   include "usbd_cdc_if.h"
#elif defined(ESP32)
#   include <Arduino.h>
#elif defined(__MINGW32__)
#   include "comservice.h"
#else
#   error Please check your platform
#endif


#ifndef GP_KEY_NAME
#   define GP_KEY_NAME(VAR) (gprotocol::str_hash((char*)__STR_DEF2__(VAR)))
#endif

#ifndef GP_KEY_STR
#   define GP_KEY_STR(STR)  (gprotocol::str_hash((char*)STR))
#endif

#ifndef GP_KEY_SIZE
#   define GP_KEY_SIZE      (32)
#endif

#ifdef DEBUG
#   define GPROTOCOL_BEDUG_GET (0)
#   define GPROTOCOL_BEDUG_SET (1)
#endif


struct gprotocol
{
private:
#if GPROTOCOL_BEDUG_GET || GPROTOCOL_BEDUG_SET
    static std::unordered_map<uint32_t, std::string> bedug_table;
#else
    static std::unordered_set<uint32_t> hashes;
#endif

public:
    using type_t = uint64_t;

    static uint32_t str_hash(const char* data)
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

#if GPROTOCOL_BEDUG_GET || GPROTOCOL_BEDUG_SET
        bedug_table.insert(
            {hash, std::string(data)}
        );
#else
        while (hashes.find(hash) != hashes.end()) {
            hash++;
        }
        hashes.insert(hash);
#endif

        return hash;
    }

private:
    std::unordered_map<uint32_t, gtuple>& table;

    void send_report(pack_t* const report)
    {
#ifdef USE_HAL_DRIVER
        CDC_Transmit_FS((uint8_t*)report, sizeof(pack_t));
#elif defined(ARDUINO)
        SERIAL_G.write(reinterpret_cast<uint8_t*>(report), sizeof(pack_t));
#elif defined(__MINGW32__)
        COMService::sendReport(*report);
#endif
    }

    uint8_t index(const uint32_t key, const uint8_t index = 0)
    {
        auto it = table.find(key);
        if (it == table.end()) {
            BEDUG_ASSERT(false, "Table not found error");
            return 0;
        }
        return it->second.index(index);
    }

    void details(
        const uint32_t key,
        const uint8_t index,
        const uint8_t* data,
        const bool is_request,
        const bool is_get
	) {
#if !GPROTOCOL_BEDUG_GET
    	if (is_get) {
    		return;
    	}
#endif
#if !GPROTOCOL_BEDUG_SET
    	if (!is_get) {
    		return;
    	}
#endif
#if GPROTOCOL_BEDUG_GET || GPROTOCOL_BEDUG_SET
        type_t debug_data = 0;
        auto itk = bedug_table.find(key);
        if (itk != bedug_table.end()) {
            debug_data = deserialize(data, sizeof(debug_data));
            pack_show(key, itk->second.c_str(), index, debug_data, is_request, is_get);
        } else {
            pack_show(key, "unknown", index, 0, is_request, is_get);
        }
#else
        (void)key;
        (void)index;
        (void)data;
        (void)is_request;
        (void)is_get;
#endif
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

        type_t tmp_data = deserialize(value, it->second.item_size());
        it->second.set(reinterpret_cast<uint8_t*>(&tmp_data), index);
    }

    void get_serialized(const uint32_t key, uint8_t* dst, const uint8_t index = 0)
    {
        auto it = table.find(key);
        if (it == table.end()) {
            BEDUG_ASSERT(false, "Table not found error");
            return;
        }

        type_t tmp_data = 0;
        it->second.get(reinterpret_cast<uint8_t*>(&tmp_data), index);
        memcpy(dst, serialize(tmp_data, it->second.item_size()), it->second.item_size());
    }


public:
    void show_table()
    {
#if GPROTOCOL_BEDUG_GET || GPROTOCOL_BEDUG_SET
        printTagLog(GPTL_TAG, "gprotocol table:");
        for (const auto& [ key, item ] : table) {
#ifdef _WIN64
            printPretty("%010u : %s\n", key, bedug_table[key].c_str());
#else
            printPretty("%010lu : %s\n", key, bedug_table[key].c_str());
#endif
        }
#endif
    }

    gprotocol(std::unordered_map<uint32_t, gtuple>& table): table(table)
    {
        show_table();
    }

    bool slave_recieve(pack_t* request)
    {
        if (request->crc != pack_crc(request)) {
#if GPROTOCOL_BEDUG_GET || GPROTOCOL_BEDUG_SET
        	printTagLog(GPTL_TAG, "crc error %u != %u", request->crc, pack_crc(request));
#endif
            return false;
        }

        pack_t response = {};
        response.key    = 0;
        response.index  = request->index;

        if (request->key == PACK_GETTER_KEY) {
            response.key   = static_cast<uint32_t>(deserialize(request->data, sizeof(response.key)));
            response.index = index(response.key, request->index);
        } else {
            response.key   = request->key;
            set_from_serialized(request->key, request->data, response.index);
        }

        details(response.key, request->index, request->data, true, request->key == PACK_GETTER_KEY);

        get_serialized(response.key, response.data, response.index);

        response.crc = pack_crc(&response);

        details(response.key, response.index, response.data, false, request->key == PACK_GETTER_KEY);

        send_report(&response);

        return true;
    }

    void master_send(const bool send, const uint32_t key, const uint8_t index = 0)
    {
        pack_t request = {};
        request.key    = PACK_GETTER_KEY;
        request.index  = index;

        if (send) {
            request.key = key;
            get_serialized(key, request.data, index);
        } else {
            memcpy(request.data, serialize(static_cast<type_t>(key), sizeof(key)), sizeof(key));
        }

        request.crc = pack_crc(&request);

        details(key, index, request.data, true, !send);

        send_report(&request);
    }

    bool master_recieve(pack_t* response)
    {
        details(response->key, response->index, response->data, false, true);

        if (response->crc != pack_crc(response)) {
            return false;
        }

        set_from_serialized(response->key, response->data, response->index);

        return true;
    }

    type_t get(const uint32_t key, const uint8_t index = 0)
    {
        const auto& it = table.find(key);
        if (it == table.end()) {
            BEDUG_ASSERT(false, "table is out of range");
            return 0;
        }
        type_t value = 0;
        it->second.get((uint8_t*)&value, index);
        return value;
    }

};


#endif
