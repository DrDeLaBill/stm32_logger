/* Copyright © 2023 Georgy E. All rights reserved. */

#include "soul.h"

#include <stdbool.h>


soul_t soul = {
	.settings_saved = true,
	.settings_updated = false
};


bool is_settings_saved()
{
	return soul.settings_saved;
}

bool is_settings_updated()
{
	return soul.settings_updated;
}

void set_settings_save_status(bool state)
{
	if (state) {
		soul.settings_updated = false;
	}
	soul.settings_saved = state;
}

void set_settings_update_status(bool state)
{
	if (state) {
		soul.settings_saved = false;
	}
	soul.settings_updated = state;
}
