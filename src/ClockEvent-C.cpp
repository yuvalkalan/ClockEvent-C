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

// void add_grid(ST7735 &display)
// {
//     for (size_t x = 4; x < ST7735_WIDTH; x += 8)
//     {
//         for (size_t y = 4; y < ST7735_HEIGHT; y += 8)
//         {
//             display.draw_pixel(x, y, ST7735_GREEN);
//         }
//     }
// }

// void ap_mode(tm &current_time, Settings &settings, ST7735 &display)
// {
//     TCPServer *state = new TCPServer();
//     if (!state)
//     {
//         return;
//     }
//     if (cyw43_arch_init())
//     {
//         return;
//     }
//     // Get notified if the user presses a key
//     auto context = cyw43_arch_async_context();
//     const char *ap_name = AP_WIFI_NAME;
//     const char *password = AP_WIFI_PASSWORD;
//     cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
//     ip4_addr_t mask;
//     IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
//     IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);
//     // Start the dhcp server
//     dhcp_server_t dhcp_server;
//     dhcp_server_init(&dhcp_server, &state->gw, &mask);
//     // Start the dns server
//     dns_server_t dns_server;
//     dns_server_init(&dns_server, &state->gw);
//     state->settings = &settings;
//     state->current_time = &current_time;
//     if (!tcp_server_open(state, ap_name))
//     {
//         return;
//     }
//     display.fill(ST7735_BLACK);
//     state->complete = false;
//     GraphicsText display_title_msg(0, 0, "AP mode!", 1);
//     GraphicsText display_info_msg(0, 0,
//                                   std::string("Name:\n") + AP_WIFI_NAME + std::string("\n\nPassword:\n") + AP_WIFI_PASSWORD,
//                                   1);
//     display_title_msg.center_x(ST7735_WIDTH / 2);
//     display_info_msg.center_x(ST7735_WIDTH / 2);
//     display_info_msg.center_y(ST7735_HEIGHT / 2);
//     while (!state->complete)
//     {
//         // check for disable wifi
//         int key = getchar_timeout_us(0);
//         if (key == 'd' || key == 'D' || settings.exist())
//         {
//             printf("Disabling wifi\n");
//             cyw43_arch_disable_ap_mode();
//             state->complete = true;
//         }
//         display.fill(ST7735_BLACK);
//         display_title_msg.draw(display, ST7735_WHITE);
//         display_info_msg.draw(display, ST7735_WHITE);
//         display.update();
//         sleep_ms(1000);
//     }
//     display.fill(ST7735_BLACK);
//     tcp_server_close(state);
//     dns_server_deinit(&dns_server);
//     dhcp_server_deinit(&dhcp_server);
//     cyw43_arch_deinit();
//     delete state;
// }

std::string tm_to_string(const tm &timeinfo)
{
    char buffer[80];
    if (timeinfo.tm_year == 0 - 1900)
        strftime(buffer, sizeof(buffer), "%d.%m\n%H:%M:%S", &timeinfo);
    else
        strftime(buffer, sizeof(buffer), "%d.%m.%y\n%H:%M:%S", &timeinfo);
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

int main()
{
    // init -------------------------------------
    int error = init_all();
    if (error)
        return error;
    sleep_ms(1000);
    printf("start!\n");
    Settings settings;
    // settings.reset();
    // settings.set_clock(Clock(0, "TITLE", tm()), 1);
    Rotary rotary(ROTARY_PIN_OUT_A, ROTARY_PIN_OUT_B, BUTTON_PIN);
    ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
    display.init_red();
    display.fill(ST7735_BLACK);
    // this code use access point to recieve settings
    // if (!settings.exist())
    // {
    //     // printf("%s", html_content);
    //     tm current_time;
    //     ap_mode(current_time, settings, display);
    //     setDS3231Time(&current_time);
    // }
    copy_DS3231_time();
    // ------------------------------------------

    Clock clocks[1 + SETTINGS_MAX_CLOCKS]; // add one for real clock
    clocks[0] = Clock(SETTINGS_CLOCK_TYPE_RAW, "Clock", get_rtc_time());
    int clocks_length = 1; // start with one to count real clock
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        if (settings.get_clock(i).exist())
            clocks[clocks_length++] = settings.get_clock(i);
    }
    for (size_t i = 0; i < clocks_length; i++)
    {
        Clock &clock = clocks[i];
        printf("clock:\n%d,%s,%s\n", clock.get_type(), clock.get_title().c_str(), tm_to_string(clock.get_timestamp()));
    }

    int clock_index = 0;
    uint8_t clock_cx = ST7735_WIDTH / 2, clock_radius = (ST7735_WIDTH > ST7735_HEIGHT ? ST7735_HEIGHT : ST7735_WIDTH) / 4, clock_cy = ST7735_HEIGHT - clock_radius - 10;
    int frames = 0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
        rotary.btn.update();
        int spins = rotary.get_spin();
        if (spins)
        {
            clock_index = ROUND_MOD(clock_index, spins, clocks_length);
        }
        if (rotary.btn.hold_down())
        {
            settings_config_main(display, rotary, settings);
        }
        // display current clock time -------------------------------
        const Clock &current_clock = clocks[clock_index];
        tm display_time = current_clock.calculate(get_rtc_time());
        display.draw_text(5, 5, (current_clock.get_title() + "\n" + tm_to_string(display_time)), ST7735_WHITE, 1);
        // ----------------------------------------------------------
        // display clock animation ----------------------------------
        uint32_t miliseconds = 0;
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
        // ----------------------------------------------------------
        // render screen --------------------------------------------
        display.update();
        display.fill(ST7735_BLACK);
        // ----------------------------------------------------------
        // calculate fps (for debugging) ----------------------------
        frames += 1;
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        // Update FPS every second
        if (deltaTime.count() >= 1.0f)
        {
            float fps = frames / deltaTime.count(); // Calculate FPS
            frames = 0;                             // Reset frame count
            lastTime = currentTime;                 // Reset time
            // Output FPS
            printf("fps: %f\n", fps);
        }
        // ----------------------------------------------------------
    }
    return 0;
}

// int main()
// {
//     init_all();
//     sleep_ms(1000);
//     Settings settings;
//     for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
//     {
//         Clock c = settings.get_clock(i);
//         printf("clock\texist: %d\ttitle: %s\ttime: [%s]\n", c.exist(), c.get_title().c_str(), tm_to_string(c.get_timestamp()).c_str());
//     }
//     printf("\n--------------------------------------------------\n");
//     settings.reset();
//     copy_DS3231_time();
//     for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
//     {
//         settings.set_clock(Clock(1, "1234567890", get_rtc_time()), i);
//         sleep_ms(1000);
//     }
//     for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
//     {
//         Clock c = settings.get_clock(i);
//         printf("clock\texist: %d\ttitle: %s\ttime: [%s]\n", c.exist(), c.get_title().c_str(), tm_to_string(c.get_timestamp()).c_str());
//     }
//     while (1)
//         ;
//     return 0;
// }