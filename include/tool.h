#pragma once

#include <stdbool.h>
#include <time.h>

#include <stddef.h>

bool string_to_time(const char* time_text, time_t* out_time);
void time_to_string(time_t value, char* out, size_t out_size);
void trim_newline(char* text);
void wait_enter(void);
