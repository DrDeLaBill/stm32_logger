/* Copyright © 2024 Georgy E. All rights reserved. */

#include "StackWatchdog.h"

#include "log.h"
#include "main.h"
#include "soul.h"
#include "utils.h"
#include "bmacro.h"


unsigned StackWatchdog::lastFree = 0;


void StackWatchdog::check()
{
	extern unsigned _ebss;
	unsigned *start, *end;
	__asm__ volatile ("mov %[end], sp" : [end] "=r" (end) : : );
	start = &_ebss;

	unsigned heap_end = 0;
	unsigned stack_end = 0;
	for (;start < end; start++) {
		if (!heap_end && (*start) == STACK_CANARY_WORD) {
			heap_end = (unsigned)start;
		}
		if (!heap_end) {
			continue;
		}
		if (*start == STACK_CANARY_WORD) {
			stack_end = (unsigned)start;
		}
	}


	if (!heap_end || !stack_end) {
		// TODO: an error is possible
	} else if (__abs_dif(lastFree, stack_end - heap_end)) {
		printTagLog(TAG, "RAM heap [0x%08X->0x%08X]     %u bytes", (unsigned)&_ebss, heap_end, __abs_dif((unsigned)&_ebss, heap_end) * sizeof(unsigned));
		printTagLog(TAG, "RAM stack[0x%08X->0x%08X(sp)] %u bytes", stack_end, (unsigned)end, __abs_dif(stack_end, (unsigned)end) * sizeof(unsigned));
		printTagLog(TAG, "RAM free: %u bytes", (stack_end - heap_end));
	}

	if (heap_end && stack_end) {
		lastFree = stack_end - heap_end;
	}

	BEDUG_ASSERT(
		lastFree && !__abs_dif(lastFree, stack_end - heap_end),
		"STACK OVERFLOW IS POSSIBLE or you didn't use a macro STACK_WATCHDOG_FILL_RAM on startup"
	);
}
