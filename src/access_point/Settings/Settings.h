#pragma once
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <cmath>
#include <time.h>
#include <string>
#include <string.h>
#include "TimeMaster/TimeMaster.h"

// settings flash buffer location
#define SETTINGS_WRITE_START (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_READ_START (SETTINGS_WRITE_START + XIP_BASE)
// -----------------------------------------------------------------------------------------
// settings clock configuration ------------------------------------------------------------
#define SETTINGS_MAX_CLOCKS 3                                                                // max number of clocks
#define SETTINGS_CLOCK_TITLE_LENGTH 10 + 1                                                   // max characters in clock title(add one byte to the 0 value at the end of the string)
#define SETTINGS_CLOCK_EXIST_OFFSET 0                                                        // is clock exist, 1 byte
#define SETTINGS_CLOCK_TYPE_OFFSET (SETTINGS_CLOCK_EXIST_OFFSET + 1)                         // type of clock counter, 1 byte
#define SETTINGS_CLOCK_TITLE_OFFSET (SETTINGS_CLOCK_TYPE_OFFSET + 1)                         // SETTINGS_CLOCK_TITLE_LENGTH bytes
#define SETTINGS_CLOCK_TM_OFFSET (SETTINGS_CLOCK_TITLE_OFFSET + SETTINGS_CLOCK_TITLE_LENGTH) // sizeof(tm) bytes
#define SETTINGS_CLOCK_SIZE (SETTINGS_CLOCK_TM_OFFSET + sizeof(tm))                          // total size of clock in bytes
// -----------------------------------------------------------------------------------------
// settings file contant offsets -----------------------------------------------------------
#define SETTINGS_EXIST_OFFSET 0                                                                        // 1 byte
#define SETTINGS_CLOCKS_ARRAY_OFFSET (SETTINGS_EXIST_OFFSET + 1)                                       // SETTINGS_CLOCK_SIZE * SETTINGS_MAX_CLOCKS  bytes
#define SETTINGS_TOTAL_SIZE (SETTINGS_CLOCKS_ARRAY_OFFSET + SETTINGS_CLOCK_SIZE * SETTINGS_MAX_CLOCKS) // total settings size
#define SETTINGS_PAGES_TOTAL_SIZE (FLASH_PAGE_SIZE * (1 + SETTINGS_TOTAL_SIZE / FLASH_PAGE_SIZE))      // total settings size fixed to next page size
// -----------------------------------------------------------------------------------------
// clock types -----------------------------------------------------------------------------
#define SETTINGS_CLOCK_TYPE_RAW 0         // show time tm, no calculation
#define SETTINGS_CLOCK_TYPE_FROM 1        // show time from tm until now
#define SETTINGS_CLOCK_TYPE_TO 2          // show time from now until tm
#define SETTINGS_CLOCK_TYPE_TO_REPEAT_Y 3 // show time from now to tm until the next annual appearance
#define SETTINGS_CLOCK_TYPE_TO_REPEAT_M 4 // show time from now to tm until the next monthly appearance
#define SETTINGS_CLOCK_TYPE_TO_REPEAT_D 5 // show time from now to tm until the next daily appearance
// -----------------------------------------------------------------------------------------

class Clock
{
private:
    bool m_exist;
    uint8_t m_type;
    std::string m_title;
    tm m_timestamp;

public:
    // constractors -----------------------------
    Clock();                                                 // default constractor
    Clock(uint8_t type, const char title[10], tm timestamp); // constractor
    Clock(const Clock &other);                               // copy constractor
    // ------------------------------------------
    // getters ----------------------------------
    bool exist() const;
    uint8_t get_type() const;
    const std::string &get_title() const;
    tm get_timestamp() const;
    tm calculate(tm current_time) const;
    // ------------------------------------------
    // setters ----------------------------------
    void set_timestamp(tm timestamp);
    // ------------------------------------------
};

class Settings
{
private:
    Clock m_clocks[SETTINGS_MAX_CLOCKS];

private:
    // file operation
    void read();
    void write();

public:
    Settings();
    // getters
    bool exist() const;
    Clock get_clock(uint8_t index) const;
    // setters
    void set_clock(const Clock &clock, uint8_t index);
    void reset();
};

static const uint8_t *settings_flash_buffer = (const uint8_t *)SETTINGS_READ_START;
