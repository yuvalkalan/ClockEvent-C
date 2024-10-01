#include "access_point/access_point.h"
#include "settings_config/settings_config.h"
// display pins -----------------------
#define ST7735_PIN_DC 9    // Data/Command
#define ST7735_PIN_RST 8   // Reset
#define ST7735_PIN_SCK 10  // SPI Clock
#define ST7735_PIN_MOSI 11 // SPI MOSI (Master Out Slave In)
#define ST7735_PIN_CS 12   // Chip Select
// ------------------------------------
// button pin -------------------------
#define BUTTON_PIN 16
#define ROTARY_PIN_OUT_A 13
#define ROTARY_PIN_OUT_B 7
// ------------------------------------
// display modes ----------------------
#define MODE_SLEEP -1
#define MODE_CLOCK 0
#define MODE_COUNTER 1
#define MODE_BIRTHDAY 2
#define MUN_OF_MODES 3
// ------------------------------------

void add_grid(ST7735 &display)
{
    for (size_t x = 4; x < ST7735_WIDTH; x += 8)
    {
        for (size_t y = 4; y < ST7735_HEIGHT; y += 8)
        {
            display.draw_pixel(x, y, ST7735_GREEN);
        }
    }
}

void ap_mode(tm &current_time, Settings &settings, ST7735 &display)
{
    TCPServer *state = new TCPServer();
    if (!state)
    {
        return;
    }

    if (cyw43_arch_init())
    {
        return;
    }

    // Get notified if the user presses a key
    auto context = cyw43_arch_async_context();

    const char *ap_name = AP_WIFI_NAME;
    const char *password = AP_WIFI_PASSWORD;

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    // Start the dhcp server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);

    // Start the dns server
    dns_server_t dns_server;
    dns_server_init(&dns_server, &state->gw);

    state->settings = &settings;

    if (!tcp_server_open(state, ap_name))
    {
        return;
    }
    display.fill(ST7735_BLACK);
    state->complete = false;
    GraphicsText display_title_msg(0, 8, "AP mode!", 2);
    GraphicsText display_info_msg(0, 0,
                                  std::string("Name:\n") + AP_WIFI_NAME + std::string("\n\nPassword:\n") + AP_WIFI_PASSWORD,
                                  2);
    display_title_msg.center_x(ST7735_WIDTH / 2);
    display_info_msg.center_x(ST7735_WIDTH / 2);
    display_info_msg.center_y(ST7735_HEIGHT / 2);
    while (!state->complete)
    {
        // check for disable wifi
        int key = getchar_timeout_us(0);
        if (key == 'd' || key == 'D' || settings.exist())
        {
            printf("Disabling wifi\n");
            cyw43_arch_disable_ap_mode();
            state->complete = true;
        }
        display.fill(ST7735_BLACK);
        display_title_msg.draw(display, ST7735_WHITE);
        display_info_msg.draw(display, ST7735_WHITE);
        display.update();
        sleep_ms(1000);
    }
    display.fill(ST7735_BLACK);
    current_time = settings.get_current_time();
    tcp_server_close(state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    delete state;
}

std::string tm_to_string(const tm &timeinfo)
{
    char buffer[80];
    if (timeinfo.tm_year == 0 - 1900)
        strftime(buffer, sizeof(buffer), "%d.%m\n%H:%M:%S", &timeinfo);
    else
        strftime(buffer, sizeof(buffer), "%d.%m.%Y\n%H:%M:%S", &timeinfo);
    return std::string(buffer);
}

static int init_all()
{
    stdio_init_all();
    int initStatus = initDS3231();
    if (initStatus)
    {
        printf("Error occurred during DS3231 initialization. %s\n", ds3231ErrorString(initStatus));
        return 1;
    }
    else
    {
        printf("DS3231 initialized.\n");
    }
    rtc_init();
    return 0;
}

bool inline tm_is_bigger(const tm &time1, const tm &time2)
{
    if (time1.tm_year == time2.tm_year)
    {
        if (time1.tm_mon == time2.tm_mon)
        {
            if (time1.tm_mday == time2.tm_mday)
            {
                if (time1.tm_hour == time2.tm_hour)
                {
                    if (time1.tm_min == time2.tm_min)
                    {
                        if (time1.tm_sec == time2.tm_sec)
                            return false;
                        return time1.tm_sec > time2.tm_sec;
                    }
                    return time1.tm_min > time2.tm_min;
                }
                return time1.tm_hour > time2.tm_hour;
            }
            return time1.tm_mday > time2.tm_mday;
        }
        return time1.tm_mon > time2.tm_mon;
    }
    return time1.tm_year > time2.tm_year;
}

int main()
{
    int error = init_all();
    if (error)
        return error;
    sleep_ms(1000);
    printf("start!\n");

    Settings settings;
    ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
    Rotary rotary(ROTARY_PIN_OUT_A, ROTARY_PIN_OUT_B, BUTTON_PIN);
    display.init_red();
    display.fill(ST7735_BLACK);
    if (!settings.exist())
    {
        // printf("%s", html_content);
        tm current_time;
        ap_mode(current_time, settings, display);
        setDS3231Time(&current_time);
    }

    copy_DS3231_time();
    // int current_sec = get_rtc_time().tm_sec;
    // while (get_rtc_time().tm_sec == current_sec)
    //     ;
    // absolute_time_t boot_time = get_absolute_time();

    tm start_time = settings.get_start_time();
    tm birthday_time = settings.get_birthday_time();
    printf("settings are:\nstart time: %s\nbirthday time: %s\n", tm_to_string(start_time).c_str(), tm_to_string(birthday_time).c_str());
    tm display_time;
    int mode = MODE_CLOCK;
    uint8_t clock_cx = ST7735_WIDTH / 2, clock_radius = (ST7735_WIDTH > ST7735_HEIGHT ? ST7735_HEIGHT : ST7735_WIDTH) / 4, clock_cy = ST7735_HEIGHT - clock_radius - 10;
    int frames = 0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            mode = ROUND_MOD(mode, spins, MUN_OF_MODES);
        }
        if (rotary.btn.clicked())
        {
            mode = mode == MODE_SLEEP ? MODE_CLOCK : MODE_SLEEP;
            display.fill(ST7735_BLACK);
            display.update();
        }
        if (rotary.btn.hold_down())
        {
            settings_config_main(display, rotary, settings);
        }
        tm current_time = get_rtc_time();
        if (mode == MODE_SLEEP)
            continue;
        if (mode == MODE_CLOCK)
        {
            display_time = current_time;
            display.draw_text(5, 5, ("Clock:\n\n" + tm_to_string(display_time)), ST7735_WHITE, 2);
        }
        else if (mode == MODE_COUNTER)
        {
            display_time = calculate_time_dif(start_time, current_time);
            display.draw_text(5, 5, ("Start:\n\n" + tm_to_string(display_time)), ST7735_WHITE, 2);
        }
        else if (mode == MODE_BIRTHDAY)
        {
            birthday_time.tm_year = current_time.tm_year;
            if (tm_is_bigger(current_time, birthday_time))
                birthday_time.tm_year += 1;

            display_time = calculate_time_dif(current_time, birthday_time);
            display.draw_text(5, 5, ("Birthday:\n\n" + tm_to_string(display_time)), ST7735_WHITE, 2);
        }
        uint32_t miliseconds = 0; //(to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(boot_time)) % 1000;
        float total_secs = display_time.tm_sec + miliseconds / 1000.0f;
        float total_mins = display_time.tm_min + total_secs / 60;
        float total_hours = display_time.tm_hour + total_mins / 60;
        float sec_angle = total_secs * 360 / 60 + 270;
        float min_angle = total_mins * 360 / 60 + 270;
        float hour_angle = total_hours * 360 / 12 + 270;
        display.draw_circle(clock_cx, clock_cy, clock_radius, 2, ST7735_WHITE);
        display.draw_line_with_angle(clock_cx, clock_cy, clock_radius * 0.85f, sec_angle, 2, ST7735_WHITE);
        display.draw_line_with_angle(clock_cx, clock_cy, clock_radius * 0.70f, min_angle, 3, ST7735_RED);
        display.draw_line_with_angle(clock_cx, clock_cy, clock_radius * 0.50f, hour_angle, 4, ST7735_BLUE);
        display.update();
        display.fill(ST7735_BLACK);

        // frames += 1;
        // auto currentTime = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<float> deltaTime = currentTime - lastTime;
        // // Update FPS every second
        // if (deltaTime.count() >= 1.0f)
        // {
        //     float fps = frames / deltaTime.count(); // Calculate FPS
        //     frames = 0;                             // Reset frame count
        //     lastTime = currentTime;                 // Reset time
        //     // Output FPS
        //     printf("fps: %f\n", fps);
        // }
    }
    return 0;
}
