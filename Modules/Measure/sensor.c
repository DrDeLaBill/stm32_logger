/* Copyright © 2024 Georgy E. All rights reserved. */

#include "sensor.h"

#include <stdint.h>
#include <stdbool.h>

#include "log.h"
#include "main.h"
#include "utils.h"
#include "gtime.h"
#include "bmacro.h"
#include "settings.h"
#include "modbus_rtu_master.h"


void _request_data_sender(uint8_t* data, uint32_t len);
void _master_internal_error_handler(void);


const char SENSOR_TAG[] = "SNR";


sensor_info_t sensor_info = {
	.modbus_initialized  = false,
	.modbus_timeout      = false,

	.modbus_errors_count = 0
};


uint8_t modbus1_char = 0;
uint8_t modbus2_char = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &MODBUS1_UART) {
        modbus_master_recieve_data_byte(modbus1_char);
        HAL_UART_Receive_IT(&MODBUS1_UART, (uint8_t*) &modbus1_char, 1);
    }
    if (huart == &MODBUS2_UART) {
    	// TODO: fast sensors
        HAL_UART_Receive_IT(&MODBUS2_UART, (uint8_t*) &modbus2_char, 1);
    }
}


void sensors_init(void (*response_packet_handler) (modbus_response_t*))
{
	BEDUG_ASSERT(response_packet_handler, "Incorrect parameter - response_packet_handler");

    modbus_master_set_request_data_sender(_request_data_sender);
    modbus_master_set_response_packet_handler(response_packet_handler);
    modbus_master_set_internal_error_handler(_master_internal_error_handler);

    HAL_UART_Receive_IT(&MODBUS1_UART, (uint8_t*) &modbus1_char, 1);
    HAL_UART_Receive_IT(&MODBUS2_UART, (uint8_t*) &modbus2_char, 1);
}

uint8_t sensors_count()
{
	uint8_t count = 0;
	for (uint8_t i = 0; i < __arr_len(settings.modbus1_status); i++) {
		sensor_status_t status = settings.modbus1_status[i];
		if (status != SETTINGS_SENSOR_EMPTY) {
			count++;
		}
	}
	return count;
}

void sensor_timeout()
{
#ifdef SENSOR_BEDUG
	printTagLog(SENSOR_TAG, "Modbus timeout");
#endif
	sensor_info.modbus_timeout = true;
	modbus_master_timeout();
}

void sensor_request_value(uint8_t index)
{
	BEDUG_ASSERT(index < __arr_len(settings.modbus1_status), "The index of MODBUS register is out of range");

	modbus_master_read_input_registers(index + 1, settings.modbus1_value_reg[index], 1);
}

void sensor_send_new_id(uint8_t old_index, uint8_t new_index)
{
	BEDUG_ASSERT(old_index < __arr_len(settings.modbus1_status), "The index of MODBUS register is out of range");
	BEDUG_ASSERT(new_index < __arr_len(settings.modbus1_status), "The index of MODBUS register is out of range");

	modbus_master_preset_single_register(old_index + 1, settings.modbus1_value_reg[old_index], new_index + 1);

	settings.modbus1_status[new_index] = settings.modbus1_status[old_index];
	settings.modbus1_id_reg[new_index] = settings.modbus1_id_reg[old_index];
	settings.modbus1_value_reg[new_index] = settings.modbus1_value_reg[old_index];

	settings.modbus1_status[old_index] = SETTINGS_SENSOR_EMPTY;
	settings.modbus1_id_reg[old_index] = 0;
	settings.modbus1_value_reg[old_index] = 0;

	set_settings_update_status(true);
}

void _request_data_sender(uint8_t* data, uint32_t len)
{
	BEDUG_ASSERT(data, "Incorrect MODBUS request data");
	if (!data) {
		return;
	}

#ifdef SENSOR_BEDUG
	print("%08lu->%s:\t", getMillis(), SENSOR_TAG);
	print("SENDED  : ");
    for (uint32_t i = 0; i < len; i++) {
    	print("%02X ", data[i]);
    }
    print("\n");
#endif
    HAL_UART_Transmit(&MODBUS1_UART, data, (uint16_t)len, GENERAL_BUS_TIMEOUT_MS);
}

void _master_internal_error_handler(void)
{
	if (sensor_info.modbus_timeout) {
		sensor_info.modbus_timeout = false;
		return;
	}
    BEDUG_ASSERT(false, "MODBUS MASTER INTERNAL ERROR");
}
