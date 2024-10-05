#pragma once
#include <ctime>
#include <sstream>
#include <iomanip>
const int MONTHS_LEN[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class Date
{
private:
    int m_year, m_month, m_day;
    int m_start_year, m_start_month, m_start_day;
    int m_month_delta, m_day_delta;

public:
    Date(int year, int month, int day);
    bool is_leap() const;
    int get_day() const;
    int get_month() const;
    int get_year() const;
    Date &operator+=(int other);
    bool operator>(const Date &other);
    bool is_equal(const Date &date2) const;
    int month_delta() const;
    bool is_bigger(const Date &date2) const;
    std::string to_string() const;
    void set_day(int day);
    void set_month(int month);
    void set_year(int year);
    int month_days() const;
};

class Time
{
private:
    int m_hour, m_min, m_sec;

public:
    Time(int hour, int min, int sec);
    Time &operator+=(int sec);
    int get_hour() const;
    int get_min() const;
    int get_sec() const;
    std::string to_string() const;
    void set_hour(int value);
    void set_min(int value);
    void set_sec(int value);
};

tm calculate_time_dif(const tm &datetime1, const tm &datetime2);
bool tm_is_bigger(const tm &time1, const tm &time2);
