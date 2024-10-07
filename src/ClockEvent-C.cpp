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
    Rotary rotary(ROTARY_PIN_OUT_A, ROTARY_PIN_OUT_B, BUTTON_PIN);
    ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
    display.init_red();
    display.fill(ST7735_BLACK);
    copy_DS3231_time();
    // ------------------------------------------
    Clock clocks[1 + SETTINGS_MAX_CLOCKS]; // add one for real clock
    clocks[0] = Clock(SETTINGS_CLOCK_TYPE_RAW, "Clock", get_rtc_time());
    int clocks_length = 1; // start with one to count real clock
    for (int i = 0; i < SETTINGS_MAX_CLOCKS; i++)
    {
        Clock clock = settings.get_clock(i);
        if (clock.exist())
        {
            clocks[clocks_length++] = clock;
        }
        std::string time_string = tm_to_string(clock.get_timestamp());
        time_string.replace(time_string.find("\n"), 1, " ");
        printf("clock:\t%s\t%d\t%s\t%s\n", clock.exist() ? "V" : "X", clock.get_type(), clock.get_title().c_str(), time_string.c_str());
    }
    printf("\n");
    for (size_t i = 0; i < clocks_length; i++)
    {
        Clock &clock = clocks[i];
        std::string time_string = tm_to_string(clock.get_timestamp());
        time_string.replace(time_string.find("\n"), 1, " ");
        printf("clock:\t%s\t%d\t%s\t%s\n", clock.exist() ? "V" : "X", clock.get_type(), clock.get_title().c_str(), time_string.c_str());
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
//     ST7735 display(ST7735_SPI_PORT, ST7735_SPI_BAUDRATE, ST7735_PIN_SCK, ST7735_PIN_MOSI, ST7735_PIN_CS, ST7735_PIN_DC, ST7735_PIN_RST);
//     display.init_red();
//     display.fill(ST7735_BLACK);
//     Rotary rotary(ROTARY_PIN_OUT_A, ROTARY_PIN_OUT_B, BUTTON_PIN);
//     copy_DS3231_time();
//     receive_string_from_user(display, rotary, "Title");

//     return 0;
// }