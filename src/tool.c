#include <tool.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* skip_leading_spaces(const char* text)
{
    const unsigned char* cursor = (const unsigned char*)text;

    while (*cursor != '\0' && isspace(*cursor))
    {
        ++cursor;
    }

    return (const char*)cursor;
}

static void flush_stdin_until_newline(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

static int is_leap_year(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int days_in_month(int year, int month)
{
    static const int days_table[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month < 1 || month > 12)
    {
        return 0;
    }

    if (month == 2 && is_leap_year(year))
    {
        return 29;
    }

    return days_table[month - 1];
}

/* 将 "YYYY-MM-DD HH:MM:SS" 文本解析为 time_t。 */
bool string_to_time(const char* time_text, time_t* out_time)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    char trailing = '\0';
    struct tm tm_time;

    if (time_text == NULL || out_time == NULL)
    {
        return false;
    }

    if (sscanf(time_text,
               "%d-%d-%d %d:%d:%d %c",
               &year,
               &month,
               &day,
               &hour,
               &minute,
               &second,
               &trailing) != 6)
    {
        return false;
    }

    if (year < 1970 || year > 3000)
    {
        return false;
    }
    if (month < 1 || month > 12)
    {
        return false;
    }
    if (day < 1 || day > days_in_month(year, month))
    {
        return false;
    }
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        return false;
    }

    memset(&tm_time, 0, sizeof(tm_time));
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

bool localtime_safe(time_t value, struct tm* out_tm)
{
    struct tm* tm_ptr;

    if (out_tm == NULL)
    {
        return false;
    }

    tm_ptr = localtime(&value);
    if (tm_ptr == NULL)
    {
        return false;
    }

    *out_tm = *tm_ptr;
    return true;
}

/* 将 time_t 格式化为字符串；无效时间输出 "-"。 */
void time_to_string(time_t value, char* out, size_t out_size)
{
    struct tm tm_value;

    if (out == NULL || out_size == 0)
    {
        return;
    }

    if (value == 0)
    {
        strncpy(out, "-", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    if (!localtime_safe(value, &tm_value))
    {
        strncpy(out, "-", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    if (strftime(out, out_size, "%Y-%m-%d %H:%M:%S", &tm_value) == 0)
    {
        strncpy(out, "-", out_size - 1);
        out[out_size - 1] = '\0';
    }
}

bool read_line_safely(const char* prompt, char* out, size_t out_size)
{
    size_t length;

    if (out == NULL || out_size < 2)
    {
        return false;
    }

    if (prompt != NULL)
    {
        printf("%s", prompt);
    }

    if (fgets(out, (int)out_size, stdin) == NULL)
    {
        return false;
    }

    length = strlen(out);
    if (length > 0 && out[length - 1] != '\n')
    {
        flush_stdin_until_newline();
    }

    trim_newline(out);
    return true;
}

bool parse_int32_strict(const char* text, int* out_value)
{
    const char* begin;
    char* endptr;
    long parsed;

    if (text == NULL || out_value == NULL)
    {
        return false;
    }

    begin = skip_leading_spaces(text);
    if (*begin == '\0')
    {
        return false;
    }

    errno = 0;
    parsed = strtol(begin, &endptr, 10);
    if (begin == endptr || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX)
    {
        return false;
    }

    endptr = (char*)skip_leading_spaces(endptr);
    if (*endptr != '\0')
    {
        return false;
    }

    *out_value = (int)parsed;
    return true;
}

bool parse_float_strict(const char* text, float* out_value)
{
    const char* begin;
    char* endptr;
    float parsed;

    if (text == NULL || out_value == NULL)
    {
        return false;
    }

    begin = skip_leading_spaces(text);
    if (*begin == '\0')
    {
        return false;
    }

    errno = 0;
    parsed = strtof(begin, &endptr);
    if (begin == endptr || errno == ERANGE || !isfinite(parsed))
    {
        return false;
    }

    endptr = (char*)skip_leading_spaces(endptr);
    if (*endptr != '\0')
    {
        return false;
    }

    *out_value = parsed;
    return true;
}

bool is_cancel_command(const char* text)
{
    const char* cursor;

    if (text == NULL)
    {
        return false;
    }

    cursor = skip_leading_spaces(text);
    if (*cursor != 'q' && *cursor != 'Q')
    {
        return false;
    }

    ++cursor;
    cursor = skip_leading_spaces(cursor);
    return *cursor == '\0';
}

/* 去除字符串末尾的 CR/LF。 */
void trim_newline(char* text)
{
    size_t len;

    if (text == 0)
    {
        return;
    }

    len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r'))
    {
        text[len - 1] = '\0';
        --len;
    }
}

/* 等待用户按回车，常用于分页停顿。 */
void wait_enter(void)
{
    int ch;

    printf("按回车键继续...");
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}
