/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_HID_HASH_H_
#define _USB_HID_HASH_H_


#include <cstdint>


constexpr uint16_t hidHash(const char* data)
{
    uint16_t  res = 0;

    for (unsigned i = 0; data[i] != '\0'; i++) {
        char symbol = data[i];
        for (unsigned j = sizeof (char) * 8; j > 0; j--) {
            res = ((res ^ symbol) & 1) ? (res >> 1) ^ 0x8C : (res >> 1);
            symbol >>= 1;
        }
    }

    return res;
}


#endif
