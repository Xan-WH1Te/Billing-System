#pragma once

#include <card_system.h>

#include <list>
#include <string>

bool saveCard(const Card& card, const std::string& filePath = "card.txt");
bool saveAllCards(const std::list<Card>& cards, const std::string& filePath = "card.txt");
std::list<Card> readCard(const std::string& filePath = "card.txt");
bool parseCard(const std::string& line, Card& outCard);
