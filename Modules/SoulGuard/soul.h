/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef __SOUL_H
#define __SOUL_H


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>


typedef struct _soul_t {
	bool settings_saved;
	bool settings_updated;
} soul_t;


extern soul_t soul;


bool is_settings_saved();
bool is_settings_updated();

void set_settings_save_status(bool state);
void set_settings_update_status(bool state);


#ifdef __cplusplus
}
#endif


#endif
