#pragma once
#include "hardware/rtc.h"
#include <hardware/watchdog.h>
#include <chrono>
#include "../graphics/graphics.h"
#include "../Rotary/Rotary.h"
#include "../access_point/Settings/Settings.h"
#include "../DS3231/DS3231.h"

#define SETTINGS_CONFIG_SELECTED_COLOR 0x1082
#define ROUND_MOD(x, y, max) ((((x) + (y)) % (max) + (max)) % (max))

#define SETTINGS_CONFIG_DAYS 0
#define SETTINGS_CONFIG_MONTHS 1
#define SETTINGS_CONFIG_YEARS 2
#define SETTINGS_CONFIG_HOURS 3
#define SETTINGS_CONFIG_MINUTES 4
#define SETTINGS_CONFIG_SECONDS 5

#define SETTINGS_CONFIG_SAVE_CANCEL 0
#define SETTINGS_CONFIG_SAVE_TRUE 1
#define SETTINGS_CONFIG_SAVE_FALSE 2
#define SETTINGS_CONFIG_SAVE_LENGTH 3

void software_reset();
void settings_config_main(ST7735 &display, Rotary &rotary, Settings &settings);
tm get_rtc_time();
void copy_DS3231_time();
