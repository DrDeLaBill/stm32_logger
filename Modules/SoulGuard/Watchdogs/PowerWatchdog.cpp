/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include <limits>

#include "usb.h"
#include "glog.h"
#include "main.h"
#include "soul.h"
#include "sensor.h"
#include "hal_defs.h"

#include "Timer.h"
#include "CodeStopwatch.h"
#include "USBController.h"


fsm::FiniteStateMachine<PowerWatchdog::fsm_table> PowerWatchdog::fsm;
utl::Timer PowerWatchdog::timer(PowerWatchdog::TIMEOUT_MS);
uint32_t PowerWatchdog::adcLevel = 0;


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == POWER_ADC.Instance)
    {
    	PowerWatchdog::stopDMA();
    }
}

void PowerWatchdog::check()
{
	utl::CodeStopwatch stopwatch("PWRw", WATCHDOG_TIMEOUT_MS);

	if (is_status(NEED_ENABLE_SENSORS)) {
		sensors_enable();
	} else {
		sensors_disable();
	}

	fsm.proccess();
}

void PowerWatchdog::_init_s::operator ()()
{
	set_error(POWER_ERROR);
	fsm.push_event(success_e{});
}

void PowerWatchdog::_wait_s::operator ()()
{
	if (usb_connected()) {
		reset_error(POWER_ERROR);
		return;
	}

	if (!timer.wait()) {
		fsm.push_event(error_e{});
	}
}

void PowerWatchdog::_check_s::operator ()() { }

void PowerWatchdog::start_DMA_a::operator ()()
{
	timer.start();

	HAL_ADC_Start_DMA(&POWER_ADC, &adcLevel, 1);
}

void PowerWatchdog::check_power_a::operator ()()
{
	uint32_t vbat = ((VOLTAGE_MULTIPLIER * REFERENSE_VOLTAGE * adcLevel) / STM_ADC_MAX);
	if ((vbat < TRIG_LEVEL_MIN || vbat > TRIG_LEVEL_MAX) && !usb_connected()) {
		fsm.push_event(error_e{});
	} else {
		reset_error(POWER_ERROR);
		fsm.push_event(success_e{});
	}
}

void PowerWatchdog::set_error_a::operator ()()
{
	set_error(POWER_ERROR);
}

void PowerWatchdog::none_a::operator ()() { }

void PowerWatchdog::stopDMA()
{
	HAL_ADC_Stop_DMA(&POWER_ADC);
	fsm.push_event(success_e{});
}
