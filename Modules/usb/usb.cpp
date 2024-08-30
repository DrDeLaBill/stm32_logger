/* Copyright © 2024 Georgy E. All rights reserved. */

#include "usb.h"

#include <cstring>
#include <unordered_map>

#include "usbd_cdc_if.h"

#include "app.h"
#include "main.h"
#include "settings.h"
#include "hal_defs.h"

#include "gprotocol.h"

#include "Timer.h"
#include "CodeStopwatch.h"


std::unordered_map<uint32_t, gtuple> table = {
    {GP_KEY_STR("dv_type"),               {reinterpret_cast<uint8_t*>(&settings.dv_type),               sizeof(settings.dv_type)}},
    {GP_KEY_STR("sw_id"),                 {reinterpret_cast<uint8_t*>(&settings.sw_id),                 sizeof(settings.sw_id)}},
    {GP_KEY_STR("fw_id"),                 {reinterpret_cast<uint8_t*>(&settings.fw_id),                 sizeof(settings.fw_id)}},
    {GP_KEY_STR("record_period"),         {reinterpret_cast<uint8_t*>(&settings.record_period),         sizeof(settings.record_period)}},
    {GP_KEY_STR("send_period"),           {reinterpret_cast<uint8_t*>(&settings.send_period),           sizeof(settings.send_period)}},
    {GP_KEY_STR("record_id"),             {reinterpret_cast<uint8_t*>(&settings.record_id),             sizeof(settings.record_id)}},
    {GP_KEY_STR("modbus1_status"),        {reinterpret_cast<uint8_t*>(&settings.modbus1_status),        sizeof(settings.modbus1_status[0]),       __arr_len(settings.modbus1_status),     modbus1_index}},
    {GP_KEY_STR("modbus1_value_reg"),     {reinterpret_cast<uint8_t*>(&settings.modbus1_value_reg),     sizeof(settings.modbus1_value_reg[0]),    __arr_len(settings.modbus1_value_reg),  modbus1_index}},
    {GP_KEY_STR("modbus1_id_reg"),        {reinterpret_cast<uint8_t*>(&settings.modbus1_id_reg),        sizeof(settings.modbus1_id_reg[0]),       __arr_len(settings.modbus1_id_reg),     modbus1_index}},
    {GP_KEY_STR("_1wire_address"),        {reinterpret_cast<uint8_t*>(&settings._1wire_address),        sizeof(settings._1wire_address[0]),       __arr_len(settings._1wire_address),     _1wire_index}},
    {GP_KEY_STR("mb1_last_id"),           {reinterpret_cast<uint8_t*>(&app_info.mb1_last_id),           sizeof(app_info.mb1_last_id)}},
    {GP_KEY_STR("mb1_new_id"),            {reinterpret_cast<uint8_t*>(&app_info.mb1_new_id),            sizeof(app_info.mb1_new_id)}},
    {GP_KEY_STR("need_mb1_id_update"),    {reinterpret_cast<uint8_t*>(&app_info.need_mb1_id_update),    sizeof(app_info.need_mb1_id_update)}},
    {GP_KEY_STR("time"),                  {reinterpret_cast<uint8_t*>(&app_info.time),                  sizeof(app_info.time)}},
    {GP_KEY_STR("need_registrate_1wire"), {reinterpret_cast<uint8_t*>(&app_info.need_registrate_1wire), sizeof(app_info.need_registrate_1wire)}},
    {GP_KEY_STR("modbus1_last_value"),    {reinterpret_cast<uint8_t*>(&app_info.modbus1_last_value),    sizeof(app_info.modbus1_last_value[0]),   __arr_len(app_info.modbus1_last_value), modbus1_index}},
    {GP_KEY_STR("_1wire_last_value"),     {reinterpret_cast<uint8_t*>(&app_info._1wire_last_value),     sizeof(app_info._1wire_last_value[0]),    __arr_len(app_info._1wire_last_value),  _1wire_index}},
    {GP_KEY_STR("_1wire_registrate"),     {reinterpret_cast<uint8_t*>(&app_info._1wire_registrate),     sizeof(app_info._1wire_registrate[0]),    __arr_len(app_info._1wire_registrate),  _1wire_index}},
};
gprotocol protocol(table);

utl::Timer gpTimer(GENERAL_TIMEOUT_MS);
utl::Timer readyTimer(5 * SECOND_MS);


bool usb_connected()
{
	return HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin);
}

bool usb_free()
{
	return !readyTimer.wait();
}

void usb_init()
{
#ifdef DEBUG
	protocol.show_table();
#endif
}

void usb_proccess()
{
	utl::CodeStopwatch stopwatch("USB", GENERAL_TIMEOUT_MS);

	if (!gpTimer.wait()) {
		memset(UserRxBufferFS, 0, sizeof(UserRxBufferFS));
		gpTimer.start();
	}

	pack_t* request = reinterpret_cast<pack_t*>(UserRxBufferFS);

	if (!request->crc) {
		return;
	}

	readyTimer.start();

	protocol.slave_recieve(request);

	memset(UserRxBufferFS, 0, sizeof(pack_t));
}
