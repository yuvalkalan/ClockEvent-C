#include "graphics.h"
// Function to linearly interpolate between two values
static uint8_t lerp(uint8_t start, uint8_t end, float precent)
{
    return start + precent * (end - start);
}
GraphicsColor::GraphicsColor(uint16_t color) : m_color(color)
{
}
uint8_t GraphicsColor::get_red() const
{
    return (m_color >> 11) & 0x1F; // Extract top 5 bits
}
uint8_t GraphicsColor::get_green() const
{
    return (m_color >> 5) & 0x3F; // Extract middle 6 bits
}
uint8_t GraphicsColor::get_blue() const
{
    return m_color & 0x1F; // Extract bottom 5 bits
}
uint16_t GraphicsColor::fade(GraphicsColor &other, float precent) const
{
    uint8_t r = lerp(get_red(), other.get_red(), precent);
    uint8_t g = lerp(get_green(), other.get_green(), precent);
    uint8_t b = lerp(get_blue(), other.get_blue(), precent);
    return GraphicsColor::makeColor(r, g, b);
}
GraphicsColor::operator uint16_t() const
{
    return m_color;
}
uint16_t GraphicsColor::makeColor(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 11) | (g << 5) | (b);
}
// GraphicsColor GraphicsColor::rgb24to16(uint32_t rgb24)
// {
//     // Extract the red, green, and blue components from the 24-bit RGB value
//     uint8_t red = (rgb24 >> 16) & 0xFF;  // Extract the upper 8 bits for red
//     uint8_t green = (rgb24 >> 8) & 0xFF; // Extract the middle 8 bits for green
//     uint8_t blue = rgb24 & 0xFF;         // Extract the lower 8 bits for blue

//     // Convert 24-bit RGB (8 bits each) to 16-bit RGB565
//     uint16_t r = (red >> 3) & 0x1F;   // 5 bits for red
//     uint16_t g = (green >> 2) & 0x3F; // 6 bits for green
//     uint16_t b = (blue >> 3) & 0x1F;  // 5 bits for blue

//     // Combine the RGB565 components into a 16-bit integer
//     return (r << 11) | (g << 5) | b;
// }

static int get_string_width(const std::string &string, uint8_t scale)
{
    int current_length = 0;
    int max_length = 0;
    for (size_t i = 0; i < string.length(); i++)
    {
        if (string[i] == '\n')
        {
            max_length = current_length > max_length ? current_length : max_length;
            current_length = 0;
        }
        else
        {
            current_length++;
        }
    }
    max_length = current_length > max_length ? current_length : max_length;
    return max_length * scale * (FONT_CHAR_WIDTH + 1) - scale;
}
static int get_string_height(const std::string &string, uint8_t scale)
{
    int num_of_lines = 1;
    for (size_t i = 0; i < string.length(); i++)
    {
        if (string[i] == '\n')
        {
            num_of_lines++;
        }
    }
    return num_of_lines * scale * (FONT_CHAR_HEIGHT + 1) - scale;
}

GraphicsRect::GraphicsRect(int x, int y, int width, int height) : m_x(x), m_y(y), m_width(width), m_height(height)
{
}
GraphicsRect::~GraphicsRect()
{
}
int GraphicsRect::top() const
{
    return m_y;
}
int GraphicsRect::bottom() const
{
    return m_y + m_height;
}
int GraphicsRect::left() const
{
    return m_x;
}
int GraphicsRect::right() const
{
    return m_x + m_width;
}
int GraphicsRect::center_x() const
{
    return m_x + m_width / 2;
}
int GraphicsRect::center_y() const
{
    return m_y + m_height / 2;
}
void GraphicsRect::top(int value)
{
    m_y = value;
}
void GraphicsRect::bottom(int value)
{
    m_y = value - m_height;
}
void GraphicsRect::left(int value)
{
    m_x = value;
}
void GraphicsRect::right(int value)
{
    m_x = value - m_width;
}
void GraphicsRect::center_x(int value)
{
    m_x = value - m_width / 2;
}
void GraphicsRect::center_y(int value)
{
    m_y = value - m_height / 2;
}
void GraphicsRect::draw(ST7735 &display, uint16_t color)
{
    for (int x = 0; x < m_width; x++)
    {
        for (int y = 0; y < m_height; y++)
        {
            display.draw_pixel(x + m_x, y + m_y, color);
        }
    }
}

GraphicsText::GraphicsText(int x, int y, std::string string, uint8_t scale) : GraphicsRect(x,
                                                                                           y,
                                                                                           get_string_width(string, scale),
                                                                                           get_string_height(string, scale)),
                                                                              m_string(string),
                                                                              m_scale(scale)
{
}
GraphicsText::~GraphicsText()
{
}
void GraphicsText::draw(ST7735 &display, uint16_t color)
{
    display.draw_text(left(), top(), m_string, color, m_scale);
}

GraphicsRect GraphicsText::get_rect() const
{
    return GraphicsRect(left(), top(), right() - left(), bottom() - top());
}
