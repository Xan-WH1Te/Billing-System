#include <tool.h>

#include <stdio.h>
#include <string.h>

/* 将 "YYYY-MM-DD HH:MM:SS" 文本解析为 time_t。 */
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

    /* 构造 tm 再交由 mktime 转换为本地时间戳。 */
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

/* 将 time_t 格式化为字符串；无效时间输出 "-"。 */
void time_to_string(time_t value, char* out, size_t out_size)
{
    struct tm* tm_ptr;

    if (out == 0 || out_size == 0)
    {
        return;
    }

    if (value == 0)
    {
        /* 0 作为“无时间”的约定值。 */
        strncpy(out, "-", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    tm_ptr = localtime(&value);
    if (tm_ptr == 0)
    {
        strncpy(out, "-", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }

    strftime(out, out_size, "%Y-%m-%d %H:%M:%S", tm_ptr);
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
