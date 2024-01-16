/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Measurer.h"

#include <cstring>

#include "log.h"
#include "main.h"
#include "utils.h"
#include "sensor.h"
#include "bmacro.h"
#include "sensor.h"
#include "settings.h"
#include "modbus_rtu_master.h"

#include "Record.h"

Record Measurer::m_record(0);
uint8_t Measurer::m_sensIdx = 0;
Measurer::ResponseStatus Measurer::m_status = RESPONSE_IDLE;


Measurer::Measurer(uint32_t delay):
	m_measureTimer(delay), m_sensorTimer(MODBUS_TIMEOUS_MS)
{
	sensors_init(response_packet_handler);
	m_status = RESPONSE_IDLE;
	m_record = Record(0);
	m_sensIdx = 0;
}

void Measurer::process()
{
	if (isStatus(RESPONSE_WAIT_SENS) && m_sensorTimer.wait()) {
		return;
	}

	if (isStatus(RESPONSE_WAIT_SENS) && !m_sensorTimer.wait()) {
	    m_record.set(m_sensIdx, SENSOR_ERROR_VALUE);
		setStatus(RESPONSE_ERROR);
		sensor_timeout();
		// TODO: errors counter for each sensor
	}

	if (m_measureTimer.wait()) {
		m_record = Record(0, sensors_count());
		return;
	}

	if (!is_settings_initialized()) {
		return;
	}

	if (m_sensIdx == __arr_len(settings.modbus1_status)) {
		printTagLog(TAG, "Write new record log");

		m_record.save();

		setStatus(RESPONSE_IDLE);
		m_measureTimer.start();
		// TODO: don't start timer if the record as not saved
		m_sensIdx = 0;
		return;
	}

	if (settings.modbus1_status[m_sensIdx] != SETTINGS_SENSOR_EMPTY) {
		sensor_request_value(m_sensIdx);
		setStatus(RESPONSE_WAIT_SENS);
		m_sensorTimer.start();
	}

	m_sensIdx++;
}

void Measurer::response_packet_handler(modbus_response_t* packet)
{
	BEDUG_ASSERT(packet, "Incorrect MODBUS response data");
	if (!packet) {
		return;
	}

    if (packet->status != MODBUS_NO_ERROR) {
#ifdef SENSOR_BEDUG
        printTagLog(TAG, "ERROR: %02x", packet->status);
#endif
        setStatus(RESPONSE_ERROR);
        return;
    }

    m_record.set(m_sensIdx + 1, packet->response[0]);
    setStatus(RESPONSE_SUCCESS);

#ifdef SENSOR_BEDUG
    printTagLog(TAG, "SUCCESS");
#endif
}

bool Measurer::isStatus(ResponseStatus responseStatus)
{
	return responseStatus == m_status;
}

void Measurer::setStatus(ResponseStatus responseStatus)
{
	m_status = responseStatus;
}
