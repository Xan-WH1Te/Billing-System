#pragma once

#include <model.h>

#include <stdbool.h>

bool card_file_save_all(const CardNode* head, const char* file_path);
CardNode* card_file_load_all(const char* file_path);
