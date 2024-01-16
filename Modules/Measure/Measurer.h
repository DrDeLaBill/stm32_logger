/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <cstdint>

#include "hal_defs.h"
#include "modbus_rtu_master.h"

#include "Timer.h"
#include "Record.h"


class Measurer
{
public:
	Measurer(uint32_t delay);
	void process();

private:
	typedef enum _ResponseStatus {
		RESPONSE_IDLE = 0,
		RESPONSE_WAIT_SENS,
		RESPONSE_WAIT_MEASURE,
		RESPONSE_SUCCESS,
		RESPONSE_ERROR
	} ResponseStatus;

	utl::Timer m_measureTimer;
	utl::Timer m_sensorTimer;

	static Record m_record;
	static uint8_t m_sensIdx;
	static ResponseStatus m_status;

	static constexpr char TAG[] = "MSR";

	static bool isStatus(ResponseStatus responseStatus);
	static void setStatus(ResponseStatus responseStatus);

	static void response_packet_handler (modbus_response_t*);
};
