#pragma once

#include <string>
#include "pico/stdlib.h"
#include "ST7735/ST7735.h"

class GraphicsColor
{
private:
    uint16_t m_color;

public:
    GraphicsColor(uint16_t color);
    uint8_t get_red() const;
    uint8_t get_green() const;
    uint8_t get_blue() const;
    uint16_t fade(GraphicsColor &other, float precent) const;
    operator uint16_t() const;
    static uint16_t make_color(uint8_t r, uint8_t g, uint8_t b);
    // static GraphicsColor rgb24to16(uint32_t rgb24);
};

class GraphicsRect
{
private:
    int m_x, m_y, m_width, m_height;

public:
    GraphicsRect(int x, int y, int width, int height);
    virtual ~GraphicsRect();
    // getters:
    int top() const;
    int bottom() const;
    int left() const;
    int right() const;
    int center_x() const;
    int center_y() const;
    // setters:
    void top(int value);
    void bottom(int value);
    void left(int value);
    void right(int value);
    void center_x(int value);
    void center_y(int value);
    // display
    virtual void draw(ST7735 &display, uint16_t color);
};

class GraphicsText : public GraphicsRect
{
private:
    std::string m_string;
    uint8_t m_scale;

public:
    GraphicsText(); // default constractor
    GraphicsText(int x, int y, std::string string, uint8_t scale);
    ~GraphicsText() override;
    void draw(ST7735 &display, uint16_t color) override;
    GraphicsRect get_rect() const;
};
