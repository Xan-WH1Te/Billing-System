#pragma once

#include <card_system.h>

#include <stdbool.h>

bool save_card(const Card* card, const char* file_path);
bool save_all_cards(const CardNode* head, const char* file_path);
CardNode* read_cards(const char* file_path);
bool parse_card_line(const char* line, Card* out_card);
