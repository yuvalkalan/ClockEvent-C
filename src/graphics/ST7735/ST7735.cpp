#include "st7735.h"

static bool inline is_inside_circle(int x, int y, int xc, int yc, int r)
{
    // Function to check if a point (x, y) lies inside a circle of radius r
    int dx = x - xc;
    int dy = y - yc;
    return (dx * dx + dy * dy) < (r * r); // Check using distance squared
}

ST7735::ST7735(spi_inst_t *spi, uint baudrate, uint sck_pin, uint mosi_pin, uint cs_pin, uint dc_pin, uint rst_pin)
    : m_spi(spi),
      m_baudrate(baudrate),
      m_sck_pin(sck_pin),
      m_mosi_pin(mosi_pin),
      m_cs_pin(cs_pin),
      m_dc_pin(dc_pin),
      m_rst_pin(rst_pin)
{
    // Initialize chosen SPI port
    spi_init(m_spi, m_baudrate);
    gpio_set_function(m_sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(m_mosi_pin, GPIO_FUNC_SPI);
    // Configure the control pins
    gpio_init(m_cs_pin);
    gpio_set_dir(m_cs_pin, GPIO_OUT);
    gpio_put(m_cs_pin, 1); // Deselect

    gpio_init(m_dc_pin);
    gpio_set_dir(m_dc_pin, GPIO_OUT);

    gpio_init(m_rst_pin);
    gpio_set_dir(m_rst_pin, GPIO_OUT);
}
void ST7735::write_command(uint8_t cmd) const
{
    // Low-level function to send commands
    gpio_put(m_dc_pin, 0); // DC low for command
    gpio_put(m_cs_pin, 0); // CS low
    spi_write_blocking(m_spi, &cmd, 1);
    gpio_put(m_cs_pin, 1); // CS high
}
void ST7735::write_data(uint8_t data) const
{
    // Low-level function to send data
    gpio_put(m_dc_pin, 1); // DC high for data
    gpio_put(m_cs_pin, 0); // CS low
    spi_write_blocking(m_spi, &data, 1);
    gpio_put(m_cs_pin, 1); // CS high
}
void ST7735::write_data_buffer(const uint8_t *buffer, size_t size) const
{
    // Write multiple data bytes
    gpio_put(m_dc_pin, 1); // DC high for data
    gpio_put(m_cs_pin, 0); // CS low
    spi_write_blocking(m_spi, buffer, size);
    gpio_put(m_cs_pin, 1); // CS high
}
void ST7735::set_addr_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) const
{
    // Set the address window for pixel updates
    // Column address set
    write_command(ST7735_CMD_CASET);
    x0 += 2;
    x1 += 2;
    y0 += 1;
    y1 += 1;
    uint8_t data[] = {0x00, x0, 0x00, x1};
    write_data_buffer(data, sizeof(data));

    // Row address set
    write_command(ST7735_CMD_RASET);
    uint8_t data2[] = {0x00, y0, 0x00, y1};
    write_data_buffer(data2, sizeof(data2));

    // Write to RAM
    write_command(ST7735_CMD_RAMWR);
}
void ST7735::update()
{
    set_addr_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);
    write_data_buffer((uint8_t *)m_buffer, ST7735_WIDTH * ST7735_HEIGHT * 2);
}
void ST7735::draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
    // Draw a pixel at (x, y) with a given color
    if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
    {
        printf("invalid pixel pos! (%d, %d)\n", x, y);
        return; // Bounds check
    }
    m_buffer[x + y * ST7735_WIDTH] = (color >> 8) | (color << 8);
}
void ST7735::fill(uint16_t color)
{
    // Fill the entire screen with a color
    color = (color >> 8) | (color << 8);
    std::fill(m_buffer, m_buffer + ST7735_WIDTH * ST7735_HEIGHT, color);
}
void ST7735::reset()
{
    // Reset the device
    gpio_put(m_dc_pin, 0);
    gpio_put(m_rst_pin, 1);
    sleep_us(500);
    gpio_put(m_rst_pin, 0);
    sleep_us(500);
    gpio_put(m_rst_pin, 1);
    sleep_us(500);
}
void ST7735::init_red()
{
    // Initialize a red tab version
    reset();
    write_command(ST7735_CMD_SWRESET); // Software reset
    sleep_us(150);
    write_command(ST7735_CMD_SLPOUT); // out of sleep mode.
    sleep_us(500);

    uint8_t data3[] = {0x01, 0x2C, 0x2D}; // fastest refresh, 6 lines front, 3 lines back.
    write_command(ST7735_CMD_FRMCTR1);    // Frame rate control.
    write_data_buffer(data3, sizeof(data3));

    write_command(ST7735_CMD_FRMCTR2); // Frame rate control.
    write_data_buffer(data3, sizeof(data3));

    uint8_t data6[] = {0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d};
    write_command(ST7735_CMD_FRMCTR3); // Frame rate control.
    write_data_buffer(data6, sizeof(data6));
    sleep_us(10);

    uint8_t data1[] = {0};
    write_command(ST7735_CMD_INVCTR); // Display inversion control
    data1[0] = 0x07;                  // Line inversion.
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_PWCTR1); // Power control
    data3[0] = 0xA2;
    data3[1] = 0x02;
    data3[2] = 0x84;
    write_data_buffer(data3, sizeof(data3));

    write_command(ST7735_CMD_PWCTR2); // Power control
    data1[0] = 0xC5;                  // VGH = 14.7V, VGL = -7.35V
    write_data_buffer(data1, sizeof(data1));

    uint8_t data2[] = {0, 0};
    write_command(ST7735_CMD_PWCTR3); // Power control
    data2[0] = 0x0A;                  // Opamp current small
    data2[1] = 0x00;                  // Boost frequency
    write_data_buffer(data2, sizeof(data2));

    write_command(ST7735_CMD_PWCTR4); // Power control
    data2[0] = 0x8A;                  // Opamp current small
    data2[1] = 0x2A;                  // Boost frequency
    write_data_buffer(data2, sizeof(data2));

    write_command(ST7735_CMD_PWCTR5); // Power control
    data2[0] = 0x8A;                  // Opamp current small
    data2[1] = 0xEE;                  // Boost frequency
    write_data_buffer(data2, sizeof(data2));

    write_command(ST7735_CMD_VMCTR1); // Power control
    data1[0] = 0x0E;
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_INVOFF);

    write_command(ST7735_CMD_MADCTL); // Power control
    data1[0] = 0x00;
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_COLMOD);
    data1[0] = 0x05;
    write_data_buffer(data1, sizeof(data1));

    write_command(ST7735_CMD_CASET); // Column address set.
    uint8_t windowLocData[] = {0x00, 0x00, 0x00, ST7735_WIDTH - 1};
    write_data_buffer(windowLocData, sizeof(windowLocData));

    write_command(ST7735_CMD_RASET); // Row address set.
    windowLocData[3] = ST7735_HEIGHT - 1;
    write_data_buffer(windowLocData, sizeof(windowLocData));

    uint8_t dataGMCTRP[] = {0x0f, 0x1a, 0x0f, 0x18, 0x2f, 0x28, 0x20, 0x22,
                            0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10};
    write_command(ST7735_CMD_GMCTRP1);
    write_data_buffer(dataGMCTRP, sizeof(dataGMCTRP));

    uint8_t dataGMCTRN[] = {0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
                            0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10};
    write_command(ST7735_CMD_GMCTRN1);
    write_data_buffer(dataGMCTRN, sizeof(dataGMCTRN));
    sleep_us(10);

    write_command(ST7735_CMD_DISPON);
    sleep_us(100);

    write_command(ST7735_CMD_NORON); // Normal display on.
    sleep_us(10);

    gpio_put(m_cs_pin, 1);
}
void ST7735::draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint8_t scale)
{
    // Ensure the character is within the bounds of the font array
    if (c < 32 || c > 126)
        return; // ASCII range check

    const uint16_t *bitmap = font_bitmap[c - 32]; // Get the bitmap for the character
    for (int i = 0; i < FONT_CHAR_HEIGHT; i++)
    { // Loop through the height of the character
        uint16_t row = bitmap[i];
        for (int j = 0; j < FONT_CHAR_WIDTH; j++)
        { // Loop through the width of the character
            if (row & (1 << j))
            {
                // printf("X");
                for (uint8_t sx = 0; sx < scale; sx++) // Draw scaled pixel
                    for (uint8_t sy = 0; sy < scale; sy++)
                        draw_pixel(x + (j * scale) + sx, y + (i * scale) + sy, color);
            }
        }
    }
}
void ST7735::draw_text(uint8_t x, uint8_t y, const std::string &text, uint16_t color, uint8_t scale)
{
    uint8_t ori_x = x;
    for (size_t i = 0; i < text.length(); i++)
    {
        if (text[i] == '\n')
        {
            x = ori_x;
            y += (FONT_CHAR_HEIGHT + 1) * scale;
        }
        else
        {
            draw_char(x, y, text[i], color, scale);
            x += (FONT_CHAR_WIDTH + 1) * scale; // Move x cursor, 5 pixels for the character + 1 pixel space
        }
    }
}
void ST7735::draw_circle(uint8_t xc, uint8_t yc, uint8_t r, uint8_t border_width, uint16_t color)
{
    int outer_radius = r + border_width; // Radius of the outer circle
    // Iterate over the bounding box of the outer circle
    for (int x = xc - outer_radius; x <= xc + outer_radius; ++x)
    {
        for (int y = yc - outer_radius; y <= yc + outer_radius; ++y)
        {
            // Check if the point is inside the outer circle but outside the inner circle
            if (is_inside_circle(x, y, xc, yc, outer_radius) && !is_inside_circle(x, y, xc, yc, r))
            {
                draw_pixel(x, y, color); // Draw the pixel if it's in the border area
            }
        }
    }
}
void ST7735::draw_line(uint8_t s_x, uint8_t s_y, uint8_t e_x, uint8_t e_y, uint8_t border_width, uint16_t color)
{
    // Bresenham's Line Drawing Algorithm with Border Width
    int dx = abs(e_x - s_x);
    int dy = abs(e_y - s_y);
    int sx = (s_x < e_x) ? 1 : -1;
    int sy = (s_y < e_y) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        // Draw the line with the specified border width by drawing multiple parallel lines
        for (int w = -border_width / 2; w <= border_width / 2; ++w)
        {
            if (dx > dy)
            {
                draw_pixel(s_x, s_y + w, color); // Horizontal thickness
            }
            else
            {
                draw_pixel(s_x + w, s_y, color); // Vertical thickness
            }
        }

        if (s_x == e_x && s_y == e_y)
            break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            s_x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            s_y += sy;
        }
    }
}
void ST7735::draw_line_with_angle(uint8_t s_x, uint8_t s_y, float length, float angle_deg, uint8_t border_width, uint16_t color)
{
    // Function to draw a line given the start position, length, and angle
    // Convert the angle from degrees to radians
    float angle_rad = angle_deg * M_PI / 180.0f;

    // Calculate the end position using trigonometry
    uint8_t e_x = s_x + length * cos(angle_rad);
    uint8_t e_y = s_y + length * sin(angle_rad);

    // Use the draw_line function to actually draw the line
    draw_line(s_x, s_y, e_x, e_y, border_width, color);
}
