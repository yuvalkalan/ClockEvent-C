#include "Settings.h"
#if LIB_PICO_STDIO_USB
void enable_usb(bool enable)
{
    stdio_set_driver_enabled(&stdio_usb, enable);
}
#else
void enable_usb(bool enable) {}
#endif

Settings::Settings() : m_current_time(), m_start_time(), m_birthday_time()
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
    memcpy(&m_current_time, settings_flash_buffer + SETTINGS_CURRENT_TIME_OFFSET, sizeof(m_current_time));
    memcpy(&m_start_time, settings_flash_buffer + SETTINGS_START_TIME_OFFSET, sizeof(m_start_time));
    memcpy(&m_birthday_time, settings_flash_buffer + SETTINGS_BIRTHDAY_TIME_OFFSET, sizeof(m_birthday_time));
}
void Settings::write()
{
    // create page --------------------
    uint8_t data[FLASH_PAGE_SIZE];
    data[SETTINGS_EXIST_OFFSET] = 1;
    memcpy(data + SETTINGS_CURRENT_TIME_OFFSET, &m_current_time, sizeof(m_current_time));
    memcpy(data + SETTINGS_START_TIME_OFFSET, &m_start_time, sizeof(m_start_time));
    memcpy(data + SETTINGS_BIRTHDAY_TIME_OFFSET, &m_birthday_time, sizeof(m_birthday_time));
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
    m_current_time = tm();
    m_start_time = tm();
    m_birthday_time = tm();
}
tm Settings::get_current_time() const
{
    return m_current_time;
}
tm Settings::get_start_time() const
{
    return m_start_time;
}
tm Settings::get_birthday_time() const
{
    return m_birthday_time;
}

void Settings::set_current_time(const tm &timestamp)
{
    m_current_time = timestamp;
    write();
}
void Settings::set_start_time(const tm &timestamp)
{
    m_start_time = timestamp;
    write();
}
void Settings::set_birthday_time(const tm &timestamp)
{
    m_birthday_time = timestamp;
    write();
}