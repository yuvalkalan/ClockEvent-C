#pragma once

#include <stdio.h>
#include <pico/stdlib.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cmath>
#include <time.h>
#include <string.h>
// settings flash buffer location
#define SETTINGS_WRITE_START (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_READ_START (SETTINGS_WRITE_START + XIP_BASE)
// ---------------------------------------------------------------------------
// default settings configuration (percents from max)
#define DEF_MAX_BRIGHT 20
#define DEF_SENSITIVITY 30
#define DEF_VOLUME_THRESHOLD 12
// ---------------------------------------------------------------------------
// settings file contant offsets
#define SETTINGS_EXIST_OFFSET 0                                                 // 1 byte
#define SETTINGS_CURRENT_TIME_OFFSET (SETTINGS_EXIST_OFFSET + sizeof(tm))       // sizeof(tm) bytes
#define SETTINGS_START_TIME_OFFSET (SETTINGS_CURRENT_TIME_OFFSET + sizeof(tm))  // sizeof(tm) bytes
#define SETTINGS_BIRTHDAY_TIME_OFFSET (SETTINGS_START_TIME_OFFSET + sizeof(tm)) // sizeof(tm) bytes
// ---------------------------------------------------------------------------
static const uint8_t *settings_flash_buffer = (const uint8_t *)SETTINGS_READ_START;
void enable_usb(bool enable);
uint8_t inline fix_percent(int value)
{
    if (value < 0)
        return 0;
    else if (value > 100)
        return 100;
    return value;
}

class Settings
{
private:
    tm m_current_time;
    tm m_start_time;
    tm m_birthday_time;

private:
    // file operation
    void
    read();
    void write();

public:
    Settings();
    // getters
    bool exist() const;
    tm get_current_time() const;
    tm get_start_time() const;
    tm get_birthday_time() const;
    // setters
    void set_current_time(const tm &timestamp);
    void set_start_time(const tm &timestamp);
    void set_birthday_time(const tm &timestamp);
    void reset();
};
