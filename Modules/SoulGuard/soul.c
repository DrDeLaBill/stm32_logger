/* Copyright © 2023 Georgy E. All rights reserved. */

#include "soul.h"

#include <stdbool.h>


soul_t soul = {
};

void STACK_WATCHDOG_FILL_RAM(void) {
	extern unsigned _ebss;
	volatile unsigned *top, *start;
	__asm__ volatile ("mov %[top], sp" : [top] "=r" (top) : : );
	start = &_ebss;
	while (start < top) {
		*(start++) = STACK_CANARY_WORD;
	}
}
