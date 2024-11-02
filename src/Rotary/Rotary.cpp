#include "Rotary.h"

Rotary *Rotary::instance = nullptr; // Define the static instance variable

void Rotary::encoder_callback(uint gpio, uint32_t events)
{
    Rotary *instance = getInstance(); // Get the current instance
    bool current_clk = gpio_get(instance->m_clk);
    bool current_dt = gpio_get(instance->m_dt);
    if (current_dt && current_clk != instance->m_last_clk) // A pin state changed
        if (current_clk == current_dt)
            instance->m_spin++; // Clockwise
        else
            instance->m_spin--;         // Counter-clockwise
    instance->m_last_clk = current_clk; // Update last state
}

Rotary *Rotary::getInstance()
{
    return instance;
}

Rotary::Rotary(int clk, int dt, int button) : m_clk(clk), m_dt(dt), m_last_clk(false), m_spin(0), btn(button)
{
    config_pins();
    m_last_clk = gpio_get(clk);
    gpio_set_irq_enabled_with_callback(clk, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &encoder_callback);
    Rotary::instance = this;
}

void Rotary::config_pins()
{
    gpio_init(m_clk);
    gpio_set_dir(m_clk, GPIO_IN);
    gpio_pull_up(m_clk);
    gpio_init(m_dt);
    gpio_set_dir(m_dt, GPIO_IN);
    gpio_pull_up(m_dt);
    btn.config_pins();
}
int Rotary::get_spin()
{
    int v = m_spin;
    m_spin = 0;
    return v;
}
