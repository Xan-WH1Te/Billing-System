#pragma once

#include <stdbool.h>
#include <time.h>

bool string_to_time(const char* time_text, time_t* out_time);
