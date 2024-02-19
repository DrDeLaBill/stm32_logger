/* Copyright © 2024 Georgy E. All rights reserved. */

#include "sensor.h"

#include <stdint.h>
#include <stdbool.h>

#include "log.h"
#include "main.h"
#include "utils.h"
#include "gtime.h"
#include "bmacro.h"
#include "hal_defs.h"
#include "settings.h"
#include "modbus_rtu_master.h"
#include "modbus_rtu_slave.h"


void _request_data_sender(uint8_t* data, uint32_t len);
void _master_internal_error_handler(void);


const char SENSOR_TAG[] = "SNS";


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
        HAL_UART_Receive_IT(&MODBUS1_UART, &modbus1_char, 1);
    }
    if (huart == &MODBUS2_UART) {
    	// TODO: fast sensors
        HAL_UART_Receive_IT(&MODBUS2_UART, &modbus2_char, 1);
    }
}


void sensors_init(void (*response_packet_handler) (modbus_response_t*))
{
	BEDUG_ASSERT(response_packet_handler, "Incorrect parameter - response_packet_handler");

    modbus_master_set_request_data_sender(_request_data_sender);
    modbus_master_set_response_packet_handler(response_packet_handler);
    modbus_master_set_internal_error_handler(_master_internal_error_handler);

    HAL_UART_Receive_IT(&MODBUS1_UART, &modbus1_char, 1);
    HAL_UART_Receive_IT(&MODBUS2_UART, &modbus2_char, 1);
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
#if SENSOR_BEDUG
	printTagLog(SENSOR_TAG, "Modbus timeout");
#endif
	sensor_info.modbus_timeout = true;
	modbus_master_timeout();
}

void sensor_request_value(uint8_t id)
{
	BEDUG_ASSERT(id < __arr_len(settings.modbus1_status), "The index of MODBUS register is out of range");

	modbus_master_read_input_registers(id + 1, settings.modbus1_value_reg[id], 1);
}

void sensor_send_new_id(uint8_t old_id, uint8_t new_id)
{
	BEDUG_ASSERT(old_id < __arr_len(settings.modbus1_status), "The index of MODBUS register is out of range");
	BEDUG_ASSERT(new_id < __arr_len(settings.modbus1_status), "The index of MODBUS register is out of range");

	modbus_master_preset_single_register(old_id + 1, settings.modbus1_value_reg[old_id], new_id + 1);

	settings.modbus1_status[new_id] = settings.modbus1_status[old_id];
	settings.modbus1_id_reg[new_id] = settings.modbus1_id_reg[old_id];
	settings.modbus1_value_reg[new_id] = settings.modbus1_value_reg[old_id];

	settings.modbus1_status[old_id] = SETTINGS_SENSOR_EMPTY;
	settings.modbus1_id_reg[old_id] = 0;
	settings.modbus1_value_reg[old_id] = 0;

	set_settings_update_status(true);
}

void _request_data_sender(uint8_t* data, uint32_t len)
{
	BEDUG_ASSERT(data, "Incorrect MODBUS request data");
	if (!data) {
		return;
	}

#if SENSOR_BEDUG
	gprint("%08lu->%s:\t", getMillis(), SENSOR_TAG);
	gprint("SENDED  : ");
    for (uint32_t i = 0; i < len; i++) {
    	gprint("%02X ", data[i]);
    }
    gprint("\n");
#endif
	HAL_GPIO_WritePin(MODBUS_EN_GPIO_Port, MODBUS_EN_Pin, GPIO_PIN_SET);
    HAL_UART_Transmit(&MODBUS1_UART, data, (uint16_t)len, GENERAL_TIMEOUT_MS);
	HAL_GPIO_WritePin(MODBUS_EN_GPIO_Port, MODBUS_EN_Pin, GPIO_PIN_RESET);
}

void _master_internal_error_handler(void)
{
	if (sensor_info.modbus_timeout) {
		sensor_info.modbus_timeout = false;
		return;
	}
    BEDUG_ASSERT(false, "MODBUS MASTER INTERNAL ERROR");
}
