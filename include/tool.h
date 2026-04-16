#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <stddef.h>

/**
 * @brief 将文本解析为本地时间戳。
 *
 * 输入格式必须为 YYYY-MM-DD HH:MM:SS。
 */
bool string_to_time(const char* time_text, time_t* out_time);

/**
 * @brief 将时间戳格式化为文本。
 *
 * 当 value 为 0 或转换失败时输出 "-"。
 */
void time_to_string(time_t value, char* out, size_t out_size);

/**
 * @brief 安全读取一行输入，自动清理超长输入残留。
 */
bool read_line_safely(const char* prompt, char* out, size_t out_size);

/**
 * @brief 严格解析十进制整数（允许首尾空白，不允许尾随垃圾字符）。
 */
bool parse_int32_strict(const char* text, int* out_value);

/**
 * @brief 严格解析十进制 64 位整数（允许首尾空白，不允许尾随垃圾字符）。
 */
bool parse_int64_strict(const char* text, int64_t* out_value);

/**
 * @brief 严格解析十进制小数并按 scale_digits 放大为整数。
 *
 * 示例：text="0.0500", scale_digits=4 => 500
 */
bool parse_decimal_to_scaled_int64(const char* text, int scale_digits, int64_t* out_value);

/**
 * @brief 严格解析浮点数（允许首尾空白，不允许尾随垃圾字符）。
 */
bool parse_float_strict(const char* text, float* out_value);

/**
 * @brief 严格解析“元”金额文本并转换为“分”。
 *
 * 支持格式：123、123.4、123.45、-5.00。
 */
bool parse_yuan_to_cent_strict(const char* text, int64_t* out_cent);

/**
 * @brief 将“分”转换为“元”的展示值。
 */
double cent_to_yuan_double(int64_t cent);

/**
 * @brief 将按 10^scale_digits 缩放的整数转换为展示浮点值。
 */
double scaled_int64_to_double(int64_t value, int scale_digits);

/**
 * @brief 判断输入是否为取消命令 q/Q（忽略空白）。
 */
bool is_cancel_command(const char* text);

/**
 * @brief 线程不安全的 localtime 包装，返回拷贝结果以避免静态存储区被覆盖。
 */
bool localtime_safe(time_t value, struct tm* out_tm);

void trim_newline(char* text);
void wait_enter(void);
