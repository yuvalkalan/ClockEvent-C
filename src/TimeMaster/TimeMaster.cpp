#include "TimeMaster.h"

Date::Date(int year, int month, int day)
    : m_year(year), m_month(month - 1), m_day(day - 1), m_start_year(year), m_start_month(month - 1), m_start_day(day - 1),
      m_month_delta(0), m_day_delta(0) {}
bool Date::is_leap() const
{
    return m_year % 4 == 0 && (m_year % 100 != 0 || m_year % 400 == 0);
}
int Date::get_day() const
{
    return m_day + 1;
}
int Date::get_month() const
{
    return m_month + 1;
}
int Date::get_year() const
{
    return m_year;
}
Date &Date::operator+=(int other)
{
    for (int i = 0; i < other; i++)
    {
        m_day += 1;
        if (m_day >= MONTHS_LEN[m_month])
        {
            if (!(m_day == 28 && m_month == 1 && is_leap()))
            {
                m_day = 0;
                m_month += 1;
                m_month_delta += 1;
            }
        }
        if (m_month == 12)
        {
            m_day = 0;
            m_month = 0;
            m_year += 1;
        }
    }
    return *this;
}
bool Date::operator>(const Date &other)
{
    if (get_year() == other.get_year())
    {
        if (get_month() == other.get_month())
        {
            if (get_day() == other.get_day())
                return false;
            return get_day() > other.get_day();
        }
        return get_month() > other.get_month();
    }
    return get_year() > other.get_year();
}
bool Date::is_equal(const Date &date2) const
{
    return get_year() == date2.get_year() && get_month() == date2.get_month() && get_day() == date2.get_day();
}
int Date::month_delta() const
{
    return m_month_delta;
}
bool Date::is_bigger(const Date &date2) const
{
    if (get_year() > date2.get_year())
    {
        return true;
    }
    else if (get_year() == date2.get_year())
    {
        if (get_month() > date2.get_month())
        {
            return true;
        }
        else if (get_month() == date2.get_month())
        {
            return get_day() > date2.get_day();
        }
    }
    return false;
}
std::string Date::to_string() const
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << (m_day + 1) << "."
        << std::setfill('0') << std::setw(2) << (m_month + 1) << "."
        << m_year;
    return oss.str();
}
void Date::set_day(int day)
{
    m_day = day;
}
void Date::set_month(int month)
{
    m_month = month;
}
void Date::set_year(int year)
{
    m_year = year;
}
int Date::month_days() const
{
    return MONTHS_LEN[m_month] + (m_month == 1 && is_leap() ? 1 : 0);
}

Time::Time(int hour, int min, int sec) : m_hour(hour), m_min(min), m_sec(sec) {}
Time &Time::operator+=(int sec)
{
    m_sec += sec;
    while (m_sec >= 60)
    {
        m_sec -= 60;

        m_min += 1;
    }
    while (m_min >= 60)
    {
        m_min -= 60;
        m_hour += 1;
    }
    while (m_hour >= 24)
        m_hour -= 24;
    return *this;
}
int Time::get_hour() const
{
    return m_hour;
}
int Time::get_min() const
{
    return m_min;
}
int Time::get_sec() const
{
    return m_sec;
}
std::string Time::to_string() const
{
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << m_hour << ":"
        << std::setw(2) << std::setfill('0') << m_min << ":"
        << std::setw(2) << std::setfill('0') << m_sec;
    return oss.str();
}
void Time::set_hour(int value)
{
    m_hour = value;
}
void Time::set_min(int value)
{
    m_min = value;
}
void Time::set_sec(int value)
{
    m_sec = value;
}

tm calculate_time_dif(const tm &datetime1, const tm &datetime2)
{
    // should be datetime1 < datetime2
    int year1 = datetime1.tm_year, month1 = datetime1.tm_mon, day1 = datetime1.tm_mday, hour1 = datetime1.tm_hour, minute1 = datetime1.tm_min, second1 = datetime1.tm_sec;
    int year2 = datetime2.tm_year, month2 = datetime2.tm_mon, day2 = datetime2.tm_mday, hour2 = datetime2.tm_hour, minute2 = datetime2.tm_min, second2 = datetime2.tm_sec;
    Date date1(year1 + 1900, month1 + 1, day1);
    Date date2(year2 + 1900, month2 + 1, day2);
    bool reverse = false;
    if (date1 > date2)
    {
        date1 = Date(year2 + 1900, month2 + 1, day2);
        date2 = Date(year1 + 1900, month1 + 1, day1);
        reverse = true;
    }

    int counter = 0;
    while (!date1.is_equal(date2))
    {
        date1 += 1;
        counter += 1;
    }

    int seconds = second2 - second1;
    int minutes = minute2 - minute1;
    int hours = hour2 - hour1;
    int days = day2 - day1;
    int years = date1.month_delta() / 12, months = date1.month_delta() % 12;

    if (seconds < 0)
    {
        seconds += 60;
        minutes -= 1;
    }
    if (minutes < 0)
    {
        minutes += 60;
        hours -= 1;
    }
    if (hours < 0)
    {
        hours += 24;
        days -= 1;
    }
    if (days < 0)
    {
        days += MONTHS_LEN[date1.get_month() - 1];
        months -= 1;
    }
    if (months < 0)
    {
        months += 12;
        years -= 1;
    }

    tm res;
    if (reverse)
    {
        res.tm_year = -years - 1900;
        res.tm_mon = -months - 1;
        res.tm_mday = -days;
        res.tm_hour = -hours;
        res.tm_min = -minutes;
        res.tm_sec = -seconds;
    }
    else
    {

        res.tm_year = years - 1900;
        res.tm_mon = months - 1;
        res.tm_mday = days;
        res.tm_hour = hours;
        res.tm_min = minutes;
        res.tm_sec = seconds;
    }
    return res;
}
