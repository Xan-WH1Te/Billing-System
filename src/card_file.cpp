#include <card_file.h>
#include <tool.h>

#include <fstream>
#include <sstream>

namespace
{
    std::string statusToText(CardStatus status);

    void writeCardLine(std::ostream& out, const Card& card)
    {
        out << card.getID() << "##"
            << card.getOwnerName() << "##"
            << card.getPin() << "##"
            << card.getBalanceCent() << "##"
            << statusToText(card.getStatus()) << "##"
            << card.getOpenTime() << "##"
            << card.getExpireTime() << "##"
            << card.getLastUseTime() << '\n';
    }

    std::string statusToText(CardStatus status)
    {
        switch (status)
        {
        case CardStatus::Active:
            return "Active";
        case CardStatus::LoggedOut:
            return "LoggedOut";
        case CardStatus::OutofDate:
            return "OutofDate";
        default:
            return "UnknownStatus";
        }
    }

    bool textToStatus(const std::string& text, CardStatus& outStatus)
    {
        if (text == "Active")
        {
            outStatus = CardStatus::Active;
            return true;
        }
        if (text == "LoggedOut")
        {
            outStatus = CardStatus::LoggedOut;
            return true;
        }
        if (text == "OutofDate")
        {
            outStatus = CardStatus::OutofDate;
            return true;
        }
        return false;
    }
}

bool saveCard(const Card& card, const std::string& filePath)
{
    std::ofstream out(filePath, std::ios::app);
    if (!out.is_open())
    {
        return false;
    }

    writeCardLine(out, card);

    return out.good();
}

bool saveAllCards(const std::vector<Card>& cards, const std::string& filePath)
{
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    for (const Card& card : cards)
    {
        writeCardLine(out, card);
        if (!out.good())
        {
            return false;
        }
    }

    return true;
}

bool parseCard(const std::string& line, Card& outCard)
{
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true)
    {
        std::size_t pos = line.find("##", start);
        if (pos == std::string::npos)
        {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, pos - start));
        start = pos + 2;
    }

    if (fields.size() != 9)
    {
        return false;
    }

    int id = 0;
    Fen balanceCent = 0;
    int inSessionNumber = 0;

    try
    {
        id = std::stoi(fields[0]);
        balanceCent = static_cast<Fen>(std::stoll(fields[3]));
        inSessionNumber = std::stoi(fields[5]);
    }
    catch (const std::exception&)
    {
        return false;
    }

    CardStatus status = CardStatus::Active;
    if (!textToStatus(fields[4], status))
    {
        return false;
    }

    std::time_t parsedTime{};
    if (!stringToTime(fields[5], parsedTime))
    {
        return false;
    }

    if (fields[6] != "永久" && !stringToTime(fields[7], parsedTime))
    {
        return false;
    }

    if (fields[7] != "-" && !stringToTime(fields[8], parsedTime))
    {
        return false;
    }

    Card card(id, fields[1], fields[2], balanceCent);
    card.setStatus(status);
    card.setOpenTime(fields[5]);
    card.setExpireTime(fields[6]);
    card.setLastUseTime(fields[7]);
    outCard = std::move(card);
    return true;
}

std::vector<Card> readCard(const std::string& filePath)
{
    std::vector<Card> cards;
    std::ifstream in(filePath);
    if (!in.is_open())
    {
        return cards;
    }

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
        {
            continue;
        }

        Card parsedCard(1000000, "temp", "123456", 0);
        if (parseCard(line, parsedCard))
        {
            cards.push_back(std::move(parsedCard));
        }
    }

    return cards;
}
