#include "Settings.h"

#if LIB_PICO_STDIO_USB
static void enable_usb(bool enable)
{
    stdio_set_driver_enabled(&stdio_usb, enable);
}
#else
void enable_usb(bool enable) {}
#endif
Clock::Clock() : m_exist(0), m_type(SETTINGS_CLOCK_TYPE_FROM), m_title(""), m_timestamp() {}
Clock::Clock(uint8_t type, const char title[10], tm timestamp) : m_exist(true), m_type(type), m_title(title), m_timestamp(timestamp) {}
Clock::Clock(const Clock &other) : m_exist(other.m_exist), m_type(other.m_type), m_title(other.m_title), m_timestamp(other.m_timestamp) {}
bool Clock::exist() const
{
    return m_exist;
}
uint8_t Clock::get_type() const
{
    return m_type;
}
const std::string &Clock::get_title() const
{
    return m_title;
}
tm Clock::get_timestamp() const
{
    return m_timestamp;
}
tm Clock::calculate(tm current_time) const
{
    if (m_type == SETTINGS_CLOCK_TYPE_RAW)
    {
        return current_time;
    }
    else if (m_type == SETTINGS_CLOCK_TYPE_FROM)
    {
        return calculate_time_dif(m_timestamp, current_time);
    }
    else if (m_type == SETTINGS_CLOCK_TYPE_TO)
    {
        return calculate_time_dif(current_time, m_timestamp);
    }
    else if (m_type == SETTINGS_CLOCK_TYPE_TO_REPEAT_Y)
    {
        tm to_return = get_timestamp();
        to_return.tm_year = current_time.tm_year;
        if (tm_is_bigger(current_time, to_return))
            to_return.tm_year++;
        return calculate_time_dif(current_time, to_return);
    }
    else if (m_type == SETTINGS_CLOCK_TYPE_TO_REPEAT_M)
    {
        tm to_return = get_timestamp();
        to_return.tm_year = current_time.tm_year;
        to_return.tm_mon = current_time.tm_mon;
        if (tm_is_bigger(current_time, to_return))
            to_return.tm_mon++;
        return calculate_time_dif(current_time, to_return);
    }
    else if (m_type == SETTINGS_CLOCK_TYPE_TO_REPEAT_D)
    {
        tm to_return = get_timestamp();
        to_return.tm_year = current_time.tm_year;
        to_return.tm_mon = current_time.tm_mon;
        to_return.tm_mday = current_time.tm_mday;
        if (tm_is_bigger(current_time, to_return))
            to_return.tm_mday++;
        return calculate_time_dif(current_time, to_return);
    }
    // this should not reach here!!
    return tm();
}
void Clock::set_timestamp(tm timestamp)
{
    m_timestamp = timestamp;
    m_exist = true;
}
void Clock::set_title(const std::string &title)
{
    m_title = title;
    m_exist = true;
}
void Clock::set_type(uint8_t type)
{
    m_type = type;
    m_exist = true;
}
Settings::Settings() : m_clocks()
{
    if (exist())
    {
        printf("settings file found!\n");
        read();
    }
    else
    {
        printf("settings file not found!\n");
    }
}
void Settings::read()
{
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        const uint8_t *clock_offset = settings_flash_buffer + SETTINGS_CLOCKS_ARRAY_OFFSET + i * SETTINGS_CLOCK_SIZE;
        if (*(clock_offset + SETTINGS_CLOCK_EXIST_OFFSET) == 1)
        {
            uint8_t type = *(clock_offset + SETTINGS_CLOCK_TYPE_OFFSET);
            char title[SETTINGS_CLOCK_TITLE_LENGTH];
            memcpy(title, clock_offset + SETTINGS_CLOCK_TITLE_OFFSET, SETTINGS_CLOCK_TITLE_LENGTH);
            tm time;
            memcpy(&time, clock_offset + SETTINGS_CLOCK_TM_OFFSET, sizeof(tm));
            m_clocks[i] = Clock(type, title, time);
        }
    }
}
void Settings::write()
{
    // create page --------------------
    uint8_t data[SETTINGS_PAGES_TOTAL_SIZE];
    data[SETTINGS_EXIST_OFFSET] = 1;
    uint8_t clock_arr[SETTINGS_MAX_CLOCKS][SETTINGS_CLOCK_SIZE];
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        clock_arr[i][SETTINGS_CLOCK_EXIST_OFFSET] = m_clocks[i].exist();
        clock_arr[i][SETTINGS_CLOCK_TYPE_OFFSET] = m_clocks[i].get_type();
        const char *title = m_clocks[i].get_title().c_str();
        memcpy(clock_arr[i] + SETTINGS_CLOCK_TITLE_OFFSET, title, SETTINGS_CLOCK_TITLE_LENGTH);
        tm time = m_clocks[i].get_timestamp();
        memcpy(clock_arr[i] + SETTINGS_CLOCK_TM_OFFSET, &time, sizeof(time));
    }
    memcpy(data + SETTINGS_CLOCKS_ARRAY_OFFSET, &clock_arr, SETTINGS_MAX_CLOCKS * SETTINGS_CLOCK_SIZE);
    // --------------------------------
    // copy page to flash -------------
    enable_usb(false);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(SETTINGS_WRITE_START, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_WRITE_START, data, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
    enable_usb(true);
    // --------------------------------
}
bool Settings::exist() const
{
    return settings_flash_buffer[SETTINGS_EXIST_OFFSET] == 1;
}

void Settings::reset()
{
    printf("reset settings!\n");
    enable_usb(false);
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(SETTINGS_WRITE_START, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    enable_usb(true);
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        m_clocks[i] = Clock();
    }
}
Clock Settings::get_clock(uint8_t index) const
{
    return m_clocks[index];
}
void Settings::set_clock(const Clock &clock, uint8_t index)
{
    m_clocks[index] = Clock(clock); // call the copy constractor
    write();
}
