#include "settings_config.h"

typedef bool (*config_func)(ST7735 &display, Rotary &rotary, Settings &settings, int index);
struct ConfigHeader
{
    int id;
    GraphicsText msg;
    config_func func;
};

void software_reset()
{
    // reset the pico
    watchdog_enable(1, 1);
    while (1)
        ;
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
static uint8_t confirm_save_changes(ST7735 &display, Rotary &rotary, Settings &settings)
{
    uint8_t exit_status = SETTINGS_CONFIG_SAVE_TRUE;
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
            exit_status = ROUND_MOD(exit_status, spins, SETTINGS_CONFIG_SAVE_LENGTH);
        if (rotary.btn.hold_down())
            finished = true;
        display.fill(ST7735_BLACK);
        title_msg.draw(display, ST7735_WHITE);
        yes_msg.draw(display, exit_status == SETTINGS_CONFIG_SAVE_TRUE ? ST7735_GREEN : ST7735_WHITE);
        no_msg.draw(display, exit_status == SETTINGS_CONFIG_SAVE_FALSE ? ST7735_GREEN : ST7735_WHITE);
        cancel_msg.draw(display, exit_status == SETTINGS_CONFIG_SAVE_CANCEL ? ST7735_GREEN : ST7735_WHITE);
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
    int current_select = 0;
    bool exit = false;
    // // check if display overflow screen
    // bool overflow_display_y = false;
    // for (size_t i = 0; i < msgs_length; i++)
    // {
    //     overflow_display_y = overflow_display_y || msgs[i].msg.bottom() > ST7735_HEIGHT;
    // }
    // printf("overflow_display_y = %d", overflow_display_y ? 1 : 0);
    while (!exit)
    {
        // input
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            current_select = ROUND_MOD(current_select, spins, msgs_length);
            // if (overflow_display_y) // if overflow, move current selected to center
            // {
            //     for (size_t i = 0; i < msgs_length; i++)
            //     {
            //         msgs[i].msg.center_y(ST7735_HEIGHT / (msgs_length) * (i + 1 - current_select));
            //     }
            // }
        }
        if (rotary.btn.clicked())
            exit = msgs[current_select].func(display, rotary, settings, msgs[current_select].id); // apply function
        // display
        for (size_t i = 0; i < msgs_length; i++)
        {
            GraphicsText msg = msgs[i].msg;
            if (i == current_select)
            {

                GraphicsRect r(0, 0, ST7735_WIDTH - 10, msg.bottom() - msg.top() + 10);
                r.center_x(msg.center_x());
                r.center_y(msg.center_y());
                r.draw(display, SETTINGS_CONFIG_SELECTED_COLOR);
            }
            msg.draw(display, ST7735_WHITE);
            draw_settings_config_separator(display, msg.bottom());
        }
        display.update();
        display.fill(ST7735_BLACK);
    }
}

static bool settings_config_exit(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    return true;
}
static bool settings_config_datetime(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    tm start_rtc_time = get_rtc_time(); // get current time
    Date date(start_rtc_time.tm_year + 1900, start_rtc_time.tm_mon + 1, start_rtc_time.tm_mday);
    std::string date_string = date.to_string();
    // get size and position of date string
    GraphicsRect date_box = GraphicsText(0, 0, date_string, 1).get_rect();
    date_box.center_x(ST7735_WIDTH / 2);
    date_box.center_y(ST7735_HEIGHT / 3);
    // create date graphics
    GraphicsText day_substring(date_box.left(), date_box.top(), date_string.substr(0, 2), 1);                   // dd
    GraphicsText dot1_substring(day_substring.right() + 2, day_substring.top(), ".", 1);                        //.
    GraphicsText mon_substring(dot1_substring.right() + 2, dot1_substring.top(), date_string.substr(3, 2), 1);  // mm
    GraphicsText dot2_substring(mon_substring.right() + 2, mon_substring.top(), ".", 1);                        //.
    GraphicsText year_substring(dot2_substring.right() + 2, dot2_substring.top(), date_string.substr(6, 4), 1); // yyyy
    // create time graphics
    Time time(start_rtc_time.tm_hour, start_rtc_time.tm_min, start_rtc_time.tm_sec);
    auto start_chrono_clock = std::chrono::steady_clock::now();
    std::string time_string = time.to_string();
    // get size and position of time string
    GraphicsRect time_box = GraphicsText(0, 0, time_string, 1).get_rect();
    time_box.center_x(ST7735_WIDTH / 2);
    time_box.center_y(ST7735_HEIGHT / 3 * 2);
    // create visual data
    GraphicsText hour_substring(time_box.left(), time_box.top(), time_string.substr(0, 2), 1);                     // hh
    GraphicsText colon1_substring(hour_substring.right() + 2, hour_substring.top(), ":", 1);                       //:
    GraphicsText min_substring(colon1_substring.right() + 2, colon1_substring.top(), time_string.substr(3, 2), 1); // mm
    GraphicsText colon2_substring(min_substring.right() + 2, min_substring.top(), ":", 1);                         //:
    GraphicsText sec_substring(colon2_substring.right() + 2, colon2_substring.top(), time_string.substr(6, 2), 1); // ss

    int current_select = SETTINGS_CONFIG_DAYS;
    const int datetime_length = 6; // day, mon, year, hour, min and sec
    auto last_toggle_time = std::chrono::steady_clock::now();
    bool toggle_current = false;
    int exit_status = SETTINGS_CONFIG_SAVE_CANCEL;

    while (!exit_status)
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
                date.set_year(1900 + ROUND_MOD(date.get_year() - 1900, spins, 201)); // 1900 - 2100
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
            mon_substring = GraphicsText(dot1_substring.right() + 2, dot1_substring.top(), date_string.substr(3, 2), 1);
            year_substring = GraphicsText(dot2_substring.right() + 2, dot2_substring.top(), date_string.substr(6, 4), 1);
        }
        if (rotary.btn.clicked())
        {
            current_select = ROUND_MOD(current_select, 1, datetime_length);
            toggle_current = false;
            last_toggle_time = std::chrono::steady_clock::now();
        }
        if (rotary.btn.hold_down())
            exit_status = confirm_save_changes(display, rotary, settings);

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
        while (delta_count >= 1)
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
        hour_substring = GraphicsText(time_box.left(), time_box.top(), time_string.substr(0, 2), 1);                     // hh
        min_substring = GraphicsText(colon1_substring.right() + 2, colon1_substring.top(), time_string.substr(3, 2), 1); // mm
        sec_substring = GraphicsText(colon2_substring.right() + 2, colon2_substring.top(), time_string.substr(6, 2), 1); // ss
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
    if (exit_status == SETTINGS_CONFIG_SAVE_TRUE)
    {
        // set ds3231 amd rtc time
        tm time_rtc = get_rtc_time();
        time_rtc.tm_hour = time.get_hour();
        time_rtc.tm_min = time.get_min();
        time_rtc.tm_sec = time.get_sec();
        time_rtc.tm_mday = date.get_day();
        time_rtc.tm_mon = date.get_month() - 1;
        time_rtc.tm_year = date.get_year() - 1900;
        setDS3231Time(&time_rtc);
        copy_DS3231_time();
    }
    return false;
}
static bool settings_config_set_clock(ST7735 &display, Rotary &rotary, Settings &settings, int index)
{
    Clock clock = settings.get_clock(index);
    tm clock_time = clock.exist() ? clock.get_timestamp() : get_rtc_time();
    Date date(clock_time.tm_year + 1900, clock_time.tm_mon + 1, clock_time.tm_mday);
    std::string date_string = date.to_string();
    // get size and position of date string
    GraphicsRect box = GraphicsText(0, 0, date_string, 1).get_rect();
    box.center_x(ST7735_WIDTH / 2);
    box.center_y(ST7735_HEIGHT / 2);
    // create visual data
    GraphicsText day_substring(box.left(), box.top(), date_string.substr(0, 2), 1);                             // dd
    GraphicsText dot1_substring(day_substring.right() + 2, day_substring.top(), ".", 1);                        //.
    GraphicsText mon_substring(dot1_substring.right() + 2, dot1_substring.top(), date_string.substr(3, 2), 1);  // mm
    GraphicsText dot2_substring(mon_substring.right() + 2, mon_substring.top(), ".", 1);                        //.
    GraphicsText year_substring(dot2_substring.right() + 2, dot2_substring.top(), date_string.substr(6, 4), 1); // yyyy

    int current_select = SETTINGS_CONFIG_DAYS;
    const int date_length = 3; // day, month and year
    auto last_time = std::chrono::steady_clock::now();
    bool toggle_current = true;
    int exit_status = SETTINGS_CONFIG_SAVE_CANCEL;

    while (!exit_status)
    {
        // input
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
                date.set_year(1900 + ROUND_MOD(date.get_year() - 1900, spins, 201)); // 1900 - 2100
                                                                                     // if change years should check day (for 29.02)
                date.set_day(ROUND_MOD(date.get_day() - 1, 0, date.month_days()));   // 0 - month_days()
            }
            date_string = date.to_string();
            day_substring = GraphicsText(box.left(), box.top(), date_string.substr(0, 2), 1);
            mon_substring = GraphicsText(dot1_substring.right() + 2, dot1_substring.top(), date_string.substr(3, 2), 1);
            year_substring = GraphicsText(dot2_substring.right() + 2, dot2_substring.top(), date_string.substr(6, 4), 1);
        }
        if (rotary.btn.clicked())
        {
            current_select = ROUND_MOD(current_select, 1, date_length);
            last_time = std::chrono::steady_clock::now();
        }
        if (rotary.btn.hold_down())
            exit_status = confirm_save_changes(display, rotary, settings);
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> delta = current_time - last_time;
        if (delta.count() >= 0.5)
        {
            toggle_current = !toggle_current;
            last_time = current_time; // Reset the last print time
        }

        day_substring.draw(display, current_select == SETTINGS_CONFIG_DAYS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        dot1_substring.draw(display, ST7735_WHITE);
        mon_substring.draw(display, current_select == SETTINGS_CONFIG_MONTHS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        dot2_substring.draw(display, ST7735_WHITE);
        year_substring.draw(display, current_select == SETTINGS_CONFIG_YEARS && !toggle_current ? ST7735_BLACK : ST7735_WHITE);
        display.update();
        display.fill(ST7735_BLACK);
    }
    if (exit_status == SETTINGS_CONFIG_SAVE_TRUE)
    {
        tm time = get_rtc_time();
        time.tm_year = date.get_year() - 1900;
        time.tm_mon = date.get_month() - 1;
        time.tm_mday = date.get_day();
        clock.set_timestamp(time);
        printf("clock is %s ; %s", clock.exist() ? "exist" : "not exist", clock.get_title());
        settings.set_clock(clock, index);
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
        current_msg.msg = GraphicsText(0, 0, clocks[i].get_title(), 1);
        current_msg.func = settings_config_set_clock;
    }
    // set exit msg
    msgs[length - 1] = {length - 1, GraphicsText(0, 0, "Exit", 1), settings_config_exit};

    // set msgs position
    for (size_t i = 0; i < length; i++)
    {
        auto &current_msg = msgs[i].msg; // just for comfort
        current_msg.center_x(ST7735_WIDTH / 2);
        current_msg.center_y(ST7735_HEIGHT / (length + 1) * (i + 1));
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
        {length++, GraphicsText(0, 0, "Date/Time", 1), settings_config_datetime},
        {length++, GraphicsText(0, 0, "Clocks", 1), settings_config_clocks},
        {length++, GraphicsText(0, 0, "Reset", 1), settings_config_reset},
        {length++, GraphicsText(0, 0, "Exit", 1), settings_config_exit},
    };
    for (size_t i = 0; i < length; i++)
    {
        msgs[i].msg.center_x(ST7735_WIDTH / 2);
        msgs[i].msg.center_y(ST7735_HEIGHT / (length + 1) * (i + 1));
    }
    display_settings_config(display, rotary, settings, msgs, length);
}

tm get_rtc_time()
{
    datetime_t t;
    rtc_get_datetime(&t);
    tm time;
    time.tm_year = t.year - 1900;
    time.tm_mon = t.month - 1;
    time.tm_mday = t.day;
    time.tm_wday = t.dotw;
    time.tm_hour = t.hour;
    time.tm_min = t.min;
    time.tm_sec = t.sec;
    return time;
}
void copy_DS3231_time()
{
    tm current_time;
    readDS3231Time(&current_time);

    datetime_t t = {
        .year = int16_t(current_time.tm_year + 1900),
        .month = int8_t(current_time.tm_mon + 1),
        .day = int8_t(current_time.tm_mday),
        .dotw = int8_t(current_time.tm_wday), // irrelevant, calculate automatically
        .hour = int8_t(current_time.tm_hour),
        .min = int8_t(current_time.tm_min),
        .sec = int8_t(current_time.tm_sec)};
    rtc_set_datetime(&t);
    sleep_ms(10); // wait some time for rtc to reset
}
