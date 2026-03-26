#include <tool.h>

#include <stdio.h>

bool string_to_time(const char* time_text, time_t* out_time)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (time_text == NULL || out_time == NULL)
    {
        return false;
    }

    if (sscanf(time_text, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
    {
        return false;
    }

    struct tm tm_time;
    tm_time.tm_year = year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min = minute;
    tm_time.tm_sec = second;
    tm_time.tm_isdst = -1;

    *out_time = mktime(&tm_time);
    return *out_time != (time_t)-1;
}
