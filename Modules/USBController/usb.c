/* Copyright © 2024 Georgy E. All rights reserved. */

#include "usb.h"

#include "main.h"
#include "hal_defs.h"


bool usb_connected()
{
	return HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin);
}
