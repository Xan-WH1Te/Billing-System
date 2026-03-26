#pragma once

#include <stdbool.h>
#include <model.h>

bool billing_file_save_all(const BillingNode* head, const char* file_path);
BillingNode* billing_file_load_all(const char* file_path);
