/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef __SOUL_H
#define __SOUL_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


#define STACK_CANARY_WORD (0xBEDAC0DE)


typedef struct _soul_t {
} soul_t;


extern soul_t soul;

/* Filling an empty area of RAM with the STACK_CANARY_WORD value */
/* For calculating the RAM fill factor  */
extern void STACK_WATCHDOG_FILL_RAM(void);


#ifdef __cplusplus
}
#endif


#endif
