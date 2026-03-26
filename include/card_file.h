#pragma once

#include <card_system.h>

#include <string>
#include <vector>

bool saveCard(const Card& card, const std::string& filePath = "card.txt");
bool saveAllCards(const std::vector<Card>& cards, const std::string& filePath = "card.txt");
std::vector<Card> readCard(const std::string& filePath = "card.txt");
bool parseCard(const std::string& line, Card& outCard);
