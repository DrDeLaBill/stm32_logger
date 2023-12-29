#ifndef SETTINGS_DB_H
#define SETTINGS_DB_H


#include "main.h"
#include "StoragePage.h"


#define SETTINGS_BEDUG (true)


class SettingsDB
{
private:
	uint32_t size;
    uint8_t* settings;

    static const char* SETTINGS_PREFIX;
    static const char* TAG;

public:
    typedef enum _SettingsStatus {
        SETTINGS_OK = 0,
        SETTINGS_ERROR
    } SettingsStatus;

    SettingsDB(uint8_t* settings, uint32_t size);

    SettingsStatus load();
    SettingsStatus save();
    SettingsStatus reset();
};


#endif
