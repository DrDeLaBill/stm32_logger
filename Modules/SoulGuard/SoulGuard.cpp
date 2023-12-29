/* Copyright © 2023 Georgy E. All rights reserved. */

#include "SoulGuard.h"

#include "soul.h"


void SoulGuard::defend()
{
	// TODO: check IWDG or another reboot

	if (is_settings_saved()) {
		// TODO:: load settings
		set_settings_save_status(false);
	} else if (is_settings_updated()) {
		// TODO: save settings
		set_settings_update_status(false);
	}
}
