#include "settings_config.h"
#include <hardware/clocks.h>
#include <hardware/rosc.h>
#include <pico/runtime_init.h>

typedef bool (*config_func)(ST7735 &display, Rotary &rotary, Settings &settings, int index);
struct ConfigHeader
{
    int id;
    GraphicsText graphic_text;
    config_func click_func;
    config_func hold_func;
};

uint orginsave = scb_hw->scr;
uint clock0 = clocks_hw->sleep_en0;
uint clock1 = clocks_hw->sleep_en1;

void reset_clocks()
{
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    scb_hw->scr = orginsave;
    clocks_hw->sleep_en0 = clock0;
    clocks_hw->sleep_en1 = clock1;
    clocks_init();
    stdio_init_all();
    printf("switch off led\n");
    uart_default_tx_wait_blocking();
}

void software_reset()
{
    // reset the pico
    reset_clocks();
    watchdog_reboot(0, 0, 0);
}

static void tm_add_sec(tm &time, int sec)
{
    Time time_obj(time.tm_hour, time.tm_min, time.tm_sec);
    time_obj += sec;
    time.tm_hour = time_obj.get_hour();
    time.tm_min = time_obj.get_min();
    time.tm_sec = time_obj.get_sec();
}
static tm receive_datetime_from_user(ST7735 &display, Rotary &rotary, const tm &start_time, bool auto_update)
{
    Date date(start_time.tm_year + 1900, start_time.tm_mon + 1, start_time.tm_mday);
    std::string date_string = date.to_string();
    // get size and position of date string
    GraphicsRect date_box = GraphicsText(0, 0, date_string, 1).get_rect();
    date_box.center_x(ST7735_WIDTH / 2);
    date_box.center_y(ST7735_HEIGHT / 3);
    // create date graphics
    GraphicsText day_substring(date_box.left(), date_box.top(), date_string.substr(0, 2), 1);               // dd
    GraphicsText dot1_substring(day_substring.right(), day_substring.top(), ".", 1);                        //.
    GraphicsText mon_substring(dot1_substring.right(), dot1_substring.top(), date_string.substr(3, 2), 1);  // mm
    GraphicsText dot2_substring(mon_substring.right(), mon_substring.top(), ".", 1);                        //.
    GraphicsText year_substring(dot2_substring.right(), dot2_substring.top(), date_string.substr(6, 4), 1); // yyyy
    // create time graphics
    Time time(start_time.tm_hour, start_time.tm_min, start_time.tm_sec);
    auto start_chrono_clock = std::chrono::steady_clock::now();
    std::string time_string = time.to_string();
    // get size and position of time string
    GraphicsRect time_box = GraphicsText(0, 0, time_string, 1).get_rect();
    time_box.center_x(ST7735_WIDTH / 2);
    time_box.center_y(ST7735_HEIGHT / 3 * 2);
    // create visual data
    GraphicsText hour_substring(time_box.left(), time_box.top(), time_string.substr(0, 2), 1);                 // hh
    GraphicsText colon1_substring(hour_substring.right(), hour_substring.top(), ":", 1);                       //:
    GraphicsText min_substring(colon1_substring.right(), colon1_substring.top(), time_string.substr(3, 2), 1); // mm
    GraphicsText colon2_substring(min_substring.right(), min_substring.top(), ":", 1);                         //:
    GraphicsText sec_substring(colon2_substring.right(), colon2_substring.top(), time_string.substr(6, 2), 1); // ss

    int current_select = SETTINGS_CONFIG_DAYS;
    const int datetime_length = 6; // day, mon, year, hour, min and sec
    auto last_toggle_time = std::chrono::steady_clock::now();
    bool toggle_current = false;
    bool exit = false;

    while (!exit)
    {
        // handle input
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            if (current_select == SETTINGS_CONFIG_DAYS)
            {
                date.set_day(ROUND_MOD(date.get_day() - 1, spins, date.month_days())); // 0 - month_days()
            }
            else if (current_select == SETTINGS_CONFIG_MONTHS)
            {
                date.set_month(ROUND_MOD(date.get_month() - 1, spins, 12)); // 0 - 11
                // if change month should check day
                date.set_day(ROUND_MOD(date.get_day() - 1, 0, date.month_days())); // 0 - month_days()
            }
            else if (current_select == SETTINGS_CONFIG_YEARS)
            {
                date.set_year(1900 + ROUND_MOD(date.get_year() - 1900, spins, 200)); // 1900 - 2099 (DS3231 limit)
                                                                                     // if change years should check day (for 29.02)
                date.set_day(ROUND_MOD(date.get_day() - 1, 0, date.month_days()));   // 0 - month_days()
            }
            else if (current_select == SETTINGS_CONFIG_HOURS)
            {
                time.set_hour(ROUND_MOD(time.get_hour(), spins, 24)); // 0 - 23
            }
            else if (current_select == SETTINGS_CONFIG_MINUTES)
            {
                time.set_min(ROUND_MOD(time.get_min(), spins, 60)); // 0 - 59
            }
            else if (current_select == SETTINGS_CONFIG_SECONDS)
            {
                time.set_sec(ROUND_MOD(time.get_sec(), spins, 60)); // 0 - 59
            }
            date_string = date.to_string();
            day_substring = GraphicsText(date_box.left(), date_box.top(), date_string.substr(0, 2), 1);
            mon_substring = GraphicsText(dot1_substring.right(), dot1_substring.top(), date_string.substr(3, 2), 1);
            year_substring = GraphicsText(dot2_substring.right(), dot2_substring.top(), date_string.substr(6, 4), 1);
        }
        if (rotary.btn.clicked())
        {
            current_select = ROUND_MOD(current_select, 1, datetime_length);
            toggle_current = false;
            last_toggle_time = std::chrono::steady_clock::now();
        }
        if (rotary.btn.hold_down())
            exit = true;

        // handle toggle
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> delta = current_time - last_toggle_time;
        if (delta.count() >= 0.5)
        {
            toggle_current = !toggle_current;
            last_toggle_time = current_time; // Reset the last toggle time
        }
        // update current seconds
        delta = current_time - start_chrono_clock;
        double delta_count = delta.count();
        while (delta_count >= 1 && auto_update)
        {
            time += 1;
            delta_count -= 1;
            start_chrono_clock = current_time;
        }
        // handle display
        //      display date
        day_substring.draw(display, current_select == SETTINGS_CONFIG_DAYS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        dot1_substring.draw(display, ST7735_WHITE);
        mon_substring.draw(display, current_select == SETTINGS_CONFIG_MONTHS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        dot2_substring.draw(display, ST7735_WHITE);
        year_substring.draw(display, current_select == SETTINGS_CONFIG_YEARS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        //      display time
        //          set time graphics
        time_string = time.to_string();
        hour_substring = GraphicsText(time_box.left(), time_box.top(), time_string.substr(0, 2), 1);                 // hh
        min_substring = GraphicsText(colon1_substring.right(), colon1_substring.top(), time_string.substr(3, 2), 1); // mm
        sec_substring = GraphicsText(colon2_substring.right(), colon2_substring.top(), time_string.substr(6, 2), 1); // ss
        //          display time
        hour_substring.draw(display, current_select == SETTINGS_CONFIG_HOURS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        colon1_substring.draw(display, ST7735_WHITE);
        min_substring.draw(display, current_select == SETTINGS_CONFIG_MINUTES && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        colon2_substring.draw(display, ST7735_WHITE);
        sec_substring.draw(display, current_select == SETTINGS_CONFIG_SECONDS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        // update screen
        display.update();
        display.fill(ST7735_BLACK);
    }
    tm new_time;
    new_time.tm_hour = time.get_hour();
    new_time.tm_min = time.get_min();
    new_time.tm_sec = time.get_sec();
    new_time.tm_mday = date.get_day();
    new_time.tm_mon = date.get_month() - 1;
    new_time.tm_year = date.get_year() - 1900;
    return new_time;
}
static std::string receive_string_from_user(ST7735 &display, Rotary &rotary, const std::string &start_string)
{
    std::string current_string(start_string);
    const char char_map[] = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // space, a-z, A-Z, 0-9
    int current_index = 0;                                                                     // index in char_map
    const int map_length = sizeof(char_map) - 1;                                               // length of char_map
    bool exit = false;
    auto last_toggle_time = std::chrono::steady_clock::now();
    bool toggle_current = true;
    while (!exit)
    {
        // handle input
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            current_index = ROUND_MOD(current_index, spins, map_length); // return index in char_map
        }
        if (rotary.btn.clicked())
        {
            if (current_string.length() < SETTINGS_CLOCK_TITLE_LENGTH - 1)
            {
                current_string += char_map[current_index]; // add current to string
                current_index = 0;                         // reset current
            }
        }
        if (rotary.btn.double_clicked())
        {
            if (current_string.length())
                current_string.pop_back(); // delete one charecter
        }
        if (rotary.btn.hold_down())
            exit = true;
        // handle toggle
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> delta = current_time - last_toggle_time;
        if (delta.count() >= 0.5)
        {
            toggle_current = !toggle_current;
            last_toggle_time = current_time; // Reset the last toggle time
        }

        // handle display
        GraphicsText display_string(5, 0, current_string, 1);
        display_string.center_y(ST7735_HEIGHT / 2);
        GraphicsText display_current(display_string.right(), display_string.top(), std::string(1, char_map[current_index]), 1);
        GraphicsText display_underscore(display_current.left(), display_current.top() + 1, std::string(1, '_'), 1);

        display_string.draw(display, ST7735_WHITE);
        if (current_string.length() < SETTINGS_CLOCK_TITLE_LENGTH - 1)
        {
            display_current.draw(display, GraphicsColor::make_color(255, 255, 255));
            if (toggle_current)
                display_underscore.draw(display, GraphicsColor::make_color(255, 255, 255));
        } // update screen
        display.update();
        display.fill(ST7735_BLACK);
    }
    return current_string;
}
static uint8_t receive_clock_type_from_user(ST7735 &display, Rotary &rotary, uint8_t start_type)
{
    uint8_t current_type = start_type - 1; // remove the raw option
    bool exit = false;
    GraphicsText types_titles[] = {
        {0, 0, "Counter", 1},
        {0, 0, "Countdown", 1},
        {0, 0, "Countdown\n(annual)", 1},
        {0, 0, "Countdown\n(monthly)", 1},
        {0, 0, "Countdown\n(daily)", 1},
    };
    for (GraphicsText &msg : types_titles)
    {
        msg.center_x(ST7735_WIDTH / 2);
        msg.center_y(ST7735_HEIGHT / 2);
    }
    while (!exit)
    {
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            current_type = ROUND_MOD(current_type, spins, SETTINGS_CLOCK_TOTAL_TYPES - 1); // remove the raw option
        }
        if (rotary.btn.hold_down())
            exit = true;
        types_titles[current_type].draw(display, ST7735_WHITE);
        display.update();
        display.fill(ST7735_BLACK);
    }
    return current_type + 1; // add one to set it to real type
}
static bool confirm_settings_reset(ST7735 &display, Rotary &rotary)
{
    bool confirmed = false;
    bool finished = false;
    GraphicsText reset_msg(0, 8, "reset\nsettings?", 1);
    reset_msg.center_x(ST7735_WIDTH / 2);

    GraphicsText yes_msg(0, 0, "yes", 1);
    yes_msg.center_x(ST7735_WIDTH / 4);
    yes_msg.center_y(ST7735_HEIGHT / 2);

    GraphicsText no_msg(0, 0, "no", 1);
    no_msg.center_x(ST7735_WIDTH / 4 * 3);
    no_msg.center_y(ST7735_HEIGHT / 2);
    while (!finished)
    {
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins % 2 != 0)
            confirmed = !confirmed;
        if (rotary.btn.hold_down())
            finished = true;
        display.fill(ST7735_BLACK);
        reset_msg.draw(display, ST7735_WHITE);
        yes_msg.draw(display, confirmed ? ST7735_RED : ST7735_WHITE);
        no_msg.draw(display, confirmed ? ST7735_WHITE : ST7735_GREEN);
        display.update();
    }
    return confirmed;
}
static uint8_t confirm_save_changes(ST7735 &display, Rotary &rotary)
{
    uint8_t exit_status = SETTINGS_CONFIG_STATUS_TRUE;
    bool finished = false;
    GraphicsText title_msg(0, 8, "Save\nChanges?", 1);
    title_msg.center_x(ST7735_WIDTH / 2);

    GraphicsText yes_msg(0, 0, "yes", 1);
    yes_msg.center_x(ST7735_WIDTH / 4);
    yes_msg.center_y(ST7735_HEIGHT / 2);

    GraphicsText no_msg(0, 0, "no", 1);
    no_msg.center_x(ST7735_WIDTH / 4 * 3);
    no_msg.center_y(ST7735_HEIGHT / 2);

    GraphicsText cancel_msg(0, 0, "cancel", 1);
    cancel_msg.center_x(ST7735_WIDTH / 2);
    cancel_msg.top(no_msg.bottom() + 16);

    while (!finished)
    {
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
            exit_status = ROUND_MOD(exit_status, spins, SETTINGS_CONFIG_STATUS_LENGTH);
        if (rotary.btn.hold_down())
            finished = true;
        display.fill(ST7735_BLACK);
        title_msg.draw(display, ST7735_WHITE);
        yes_msg.draw(display, exit_status == SETTINGS_CONFIG_STATUS_TRUE ? ST7735_GREEN : ST7735_WHITE);
        no_msg.draw(display, exit_status == SETTINGS_CONFIG_STATUS_FALSE ? ST7735_GREEN : ST7735_WHITE);
        cancel_msg.draw(display, exit_status == SETTINGS_CONFIG_STATUS_CANCEL ? ST7735_GREEN : ST7735_WHITE);
        display.update();
    }
    return exit_status;
}

static void draw_settings_config_separator(ST7735 &display, uint8_t y)
{
    GraphicsColor start_color = SETTINGS_CONFIG_SELECTED_COLOR;
    GraphicsColor end_color = ST7735_WHITE;
    for (float x = 10; x < ST7735_WIDTH - 10; x++)
    {
        if (x < ST7735_WIDTH / 2)
            display.draw_pixel(x, y, start_color.fade(end_color, (x - 10) / (ST7735_WIDTH - 20)));
        else
            display.draw_pixel(x, y, end_color.fade(start_color, (x - 10) / (ST7735_WIDTH - 20)));
    }
}
static void display_settings_config(ST7735 &display, Rotary &rotary, Settings &settings, ConfigHeader *msgs, int msgs_length)
{
    /**
     * @brief display settings page to display
     */
    int current_select = 0;
    bool exit = false;
    // check if display overflow screen
    bool overflow_display_y = false;
    for (size_t i = 0; i < msgs_length; i++)
    {
        overflow_display_y = overflow_display_y || msgs[i].graphic_text.bottom() > ST7735_HEIGHT;
    }
    while (!exit)
    {
        // input
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            current_select = ROUND_MOD(current_select, spins, msgs_length);
            if (overflow_display_y) // if overflow, move current selected to top
            {
                for (size_t i = 0; i < msgs_length; i++)
                {
                    msgs[i].graphic_text.center_y(ST7735_HEIGHT / (SETTINGS_CONFIG_MAX_ROWS + 1) * (i + 1 - std::min(current_select, msgs_length - SETTINGS_CONFIG_MAX_ROWS)));
                }
            }
        }
        if (rotary.btn.clicked())
        {
            exit = msgs[current_select].click_func(display, rotary, settings, msgs[current_select].id); // apply function
        }
        else if (rotary.btn.hold_down())
        {
            exit = msgs[current_select].hold_func(display, rotary, settings, msgs[current_select].id);
        }
        // display
        for (size_t i = 0; i < msgs_length; i++)
        {
            GraphicsText &msg = msgs[i].graphic_text;
            if (i == current_select)
            {

                GraphicsRect r(0, 0, ST7735_WIDTH - 10, msg.bottom() - msg.top() + 10);
                r.center_x(msg.center_x());
                r.center_y(msg.center_y());
                r.draw(display, SETTINGS_CONFIG_SELECTED_COLOR);
            }
            const int top_row = std::min(current_select, msgs_length - SETTINGS_CONFIG_MAX_ROWS);
            if (!overflow_display_y || top_row <= i && i < top_row + 4)
            {
                msg.draw(display, ST7735_WHITE);
                draw_settings_config_separator(display, msg.bottom());
            }
        }
        display.update();
        display.fill(ST7735_BLACK);
    }
}

static bool settings_config_ignore(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    return false;
}
static bool settings_config_exit(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    return true;
}
static bool settings_config_datetime(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    int exit_status = SETTINGS_CONFIG_STATUS_CANCEL;
    tm user_input_datetime = get_rtc_time();
    while (!exit_status)
    {

        user_input_datetime = receive_datetime_from_user(display, rotary, user_input_datetime, true);
        auto start_time = std::chrono::steady_clock::now();
        exit_status = confirm_save_changes(display, rotary);
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> delta = end_time - start_time;
        tm_add_sec(user_input_datetime, delta.count());
    }
    if (exit_status == SETTINGS_CONFIG_STATUS_TRUE)
    {
        // set ds3231 amd rtc time
        setDS3231Time(&user_input_datetime);
    }
    return false;
}
static bool settings_config_remove_clock(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    Clock clock = settings.get_clock(index);
    if (!clock.exist())
        return false;
    bool exit_status = false;
    bool finished = false;
    GraphicsText title_msg(0, 8, "Delete\nClock?", 1);
    title_msg.center_x(ST7735_WIDTH / 2);

    GraphicsText yes_msg(0, 0, "yes", 1);
    yes_msg.center_x(ST7735_WIDTH / 4);
    yes_msg.center_y(ST7735_HEIGHT / 2);

    GraphicsText no_msg(0, 0, "no", 1);
    no_msg.center_x(ST7735_WIDTH / 4 * 3);
    no_msg.center_y(ST7735_HEIGHT / 2);
    while (!finished)
    {
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins % 2 != 0)
            exit_status = !exit_status;
        if (rotary.btn.hold_down())
            finished = true;
        display.fill(ST7735_BLACK);
        title_msg.draw(display, ST7735_WHITE);
        yes_msg.draw(display, exit_status ? ST7735_GREEN : ST7735_WHITE);
        no_msg.draw(display, !exit_status ? ST7735_GREEN : ST7735_WHITE);
        display.update();
    }
    if (exit_status)
    {
        settings.set_clock(Clock(), index); // set clock to defualt clock
        software_reset();
    }
    return false;
}
static bool settings_config_set_clock(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    Clock clock = settings.get_clock(index);
    std::string title = receive_string_from_user(display, rotary, clock.get_title());
    uint8_t clock_type = receive_clock_type_from_user(display, rotary, clock.get_type());
    int exit_status = SETTINGS_CONFIG_STATUS_CANCEL;
    tm user_input_datetime = clock.exist() ? clock.get_timestamp() : get_rtc_time();
    while (!exit_status)
    {
        user_input_datetime = receive_datetime_from_user(display, rotary, user_input_datetime, false);
        exit_status = confirm_save_changes(display, rotary);
    }
    if (exit_status == SETTINGS_CONFIG_STATUS_TRUE)
    {
        printf("clock (%d): %s, %d", index, title.c_str(), clock_type);
        clock.set_title(title);
        clock.set_timestamp(user_input_datetime);
        clock.set_type(clock_type);
        settings.set_clock(clock, index);
        software_reset(); // reset for clock to take place
    }
    return false;
}
static bool settings_config_clocks(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    // init clocks array
    Clock clocks[SETTINGS_MAX_CLOCKS];
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        if (settings.get_clock(i).exist())
            clocks[i] = settings.get_clock(i);
        else
            clocks[i] = Clock(SETTINGS_CLOCK_TYPE_RAW, "+", get_rtc_time());
    }
    // init msgs array
    const int length = SETTINGS_MAX_CLOCKS + 1;
    ConfigHeader msgs[length];
    // set clocks msgs
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        auto &current_msg = msgs[i]; // just for comfort
        current_msg.id = i;
        current_msg.graphic_text = GraphicsText(0, 0, clocks[i].get_title(), 1);
        current_msg.click_func = settings_config_set_clock;
        current_msg.hold_func = settings_config_remove_clock;
    }
    // set exit msg
    msgs[length - 1] = {length - 1, GraphicsText(0, 0, "Exit", 1), settings_config_exit, settings_config_ignore};

    // set msgs position
    for (size_t i = 0; i < length; i++)
    {
        auto &current_msg = msgs[i].graphic_text; // just for comfort
        current_msg.center_x(ST7735_WIDTH / 2);
        current_msg.center_y(ST7735_HEIGHT / (SETTINGS_CONFIG_MAX_ROWS + 1) * (i + 1));
    }
    display_settings_config(display, rotary, settings, msgs, length);
    return false;
}
static bool settings_config_reset(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    if (confirm_settings_reset(display, rotary))
    {
        settings.reset();
        software_reset();
    }
    return false;
}
void settings_config_main(ST7735 &display, Rotary &rotary, Settings &settings)
{
    int length = 0;
    ConfigHeader msgs[] = {
        {length++, GraphicsText(0, 0, "Date/Time", 1), settings_config_datetime, settings_config_ignore},
        {length++, GraphicsText(0, 0, "Clocks", 1), settings_config_clocks, settings_config_ignore},
        {length++, GraphicsText(0, 0, "Reset", 1), settings_config_reset, settings_config_ignore},
        {length++, GraphicsText(0, 0, "Exit", 1), settings_config_exit, settings_config_ignore},
    };
    for (size_t i = 0; i < length; i++)
    {
        msgs[i].graphic_text.center_x(ST7735_WIDTH / 2);
        msgs[i].graphic_text.center_y(ST7735_HEIGHT / (SETTINGS_CONFIG_MAX_ROWS + 1) * (i + 1));
    }
    display_settings_config(display, rotary, settings, msgs, length);
}

tm get_rtc_time()
{
    tm current_time;
    readDS3231Time(&current_time);
    return current_time;
}
