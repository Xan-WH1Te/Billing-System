#pragma once

#include <model.h>

#include <stdbool.h>
#include <stddef.h>

bool money_file_append(const Money* record, const char* file_path);
bool money_file_load_all(const char* file_path, Money** out_records, size_t* out_count);
void money_file_free_all(Money* records);
