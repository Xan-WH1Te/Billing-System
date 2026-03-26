#include <card_system.h>
#include <card_file.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

static std::vector<Card> g_cards;

static void persistAllCardsToFile()
{
    if (!saveAllCards(g_cards, "card.txt"))
    {
        std::cout << "警告：卡信息保存到文件失败。\n";
    }
}

void CardService::loadCardsFromFile()
{
    g_cards = readCard("card.txt");
}

static std::string getCurrentDateTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
    if (std::tm* tmPtr = std::localtime(&now))
    {
        localTime = *tmPtr;
    }

    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

enum class InputReadResult
{
    Success,
    Cancelled,
    Invalid
};

static std::string trim(const std::string& text)
{
    size_t left = 0;
    size_t right = text.size();

    while (left < right && std::isspace(static_cast<unsigned char>(text[left])) != 0)
    {
        ++left;
    }
    while (right > left && std::isspace(static_cast<unsigned char>(text[right - 1])) != 0)
    {
        --right;
    }

    return text.substr(left, right - left);
}

static bool isCancelInput(const std::string& input)
{
    std::string text = trim(input);
    return text.size() == 1 && (text[0] == 'q' || text[0] == 'Q');
}

static void printCancelledAndBackToMenu()
{
    std::cout << "已取消，返回菜单。\n";
}

bool isValidPin(const std::string& pin)
{
    if (pin.length() != 6)
        return false;
    if (!std::all_of(pin.begin(), pin.end(),
        [](unsigned char ch) { return std::isdigit(ch) != 0; }))
        return false;
    
    for (size_t i = 0; i + 2 < pin.length(); ++i)
    {
        if (pin[i] == pin[i+1] && pin[i+1] == pin[i+2])
            return false;
    }
    
    return true;
}

static bool isValidOwnerName(const std::string& owner)
{
    if (owner.empty() || owner.size() > 7)
        return false;

    return std::all_of(owner.begin(), owner.end(),
        [](unsigned char ch) { return std::isalnum(ch) != 0; });
}

void waitEnter()
{
    std::cout << "按回车键继续...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

Card::Card(int id, std::string owner, const std::string& pin, Fen initBalance)
    : id_(id), ownerName_(std::move(owner)), balanceCent_(initBalance), pin_(pin), openTime_(getCurrentDateTimeString())
{
    if (id <= 0) throw std::invalid_argument("卡号必须为7位正整数");
    if (!isValidOwnerName(ownerName_)) throw std::invalid_argument("姓名不合法");
    if (initBalance < 0) throw std::invalid_argument("初始余额不能为负");
    if (!isValidPin(pin_)) throw std::invalid_argument("密码不合法,请检查再试");
}

void Card::markLastUseNow()
{
    lastUseTime_ = getCurrentDateTimeString();
}

static InputReadResult readInt(const std::string& prompt, int& outValue)
{
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line))
    {
        std::cin.clear();
        std::cout << "输入无效，请输入纯数字。\n";
        return InputReadResult::Invalid;
    }

    if (isCancelInput(line))
    {
        return InputReadResult::Cancelled;
    }

    std::string text = trim(line);
    if (text.empty() || !std::all_of(text.begin(), text.end(),
        [](unsigned char ch) { return std::isdigit(ch) != 0; }))
    {
        std::cout << "输入无效，请输入纯数字。\n";
        return InputReadResult::Invalid;
    }

    long long value = 0;
    try
    {
        value = std::stoll(text);
    }
    catch (const std::exception&)
    {
        std::cout << "输入无效，请输入纯数字。\n";
        return InputReadResult::Invalid;
    }

    if (value > std::numeric_limits<int>::max())
    {
        std::cout << "输入超出范围，请输入有效整数。\n";
        return InputReadResult::Invalid;
    }

    outValue = static_cast<int>(value);
    return InputReadResult::Success;
}

static InputReadResult readAmountCent(const std::string& prompt, Fen& outCent)
{
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line))
    {
        std::cin.clear();
        std::cout << "金额输入无效。\n";
        return InputReadResult::Invalid;
    }

    if (isCancelInput(line))
    {
        return InputReadResult::Cancelled;
    }

    std::string text = trim(line);
    if (text.empty())
    {
        std::cout << "金额输入无效。\n";
        return InputReadResult::Invalid;
    }

    std::stringstream ss(text);
    double amountInput = 0.0;
    char extra = '\0';
    if (!(ss >> amountInput) || (ss >> extra))
    {
        std::cout << "金额输入无效。\n";
        return InputReadResult::Invalid;
    }

    if (amountInput < 0)
    {
        std::cout << "金额不能为负。\n";
        return InputReadResult::Invalid;
    }

    outCent = static_cast<Fen>(std::llround(amountInput * 100.0));
    return InputReadResult::Success;
}

static bool cardExists(int id)
{
    return std::any_of(g_cards.begin(), g_cards.end(), [id](const Card& c) { return c.getID() == id; });
}

static Card* findCardByID(int id)
{
    auto it = std::find_if(g_cards.begin(), g_cards.end(), [id](const Card& c) { return c.getID() == id; });
    if (it == g_cards.end()) return nullptr;
    return &(*it);
}

static std::string statusToString(CardStatus status)
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

static std::string formatCentToFen(Fen cent)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << static_cast<double>(cent) / 100.0;
    return out.str();
}

void CardService::addCard()
{
    int id = 0;
    while (true)
    {
        InputReadResult result = readInt("[添加卡] 请输入卡号<7位整数>：", id);
        if (result == InputReadResult::Cancelled)
        {
            printCancelledAndBackToMenu();
            return;
        }
        if (result != InputReadResult::Success)
        {
            continue;
        }
        if (id > 9999999 || id <= 999999)
        {
            std::cout << "卡号无效，请输入7位整数。\n";
            continue;
        }
        if (cardExists(id))
        {
            std::cout << "添加失败：卡号已存在。\n";
            continue;
        }
        break;
    }

    std::string owner;
    while (true)
    {
        std::cout << "[添加卡] 请输入姓名<长度不能超过7>：";
        std::getline(std::cin, owner);
        if (isCancelInput(owner))
        {
            printCancelledAndBackToMenu();
            return;
        }
        if (!isValidOwnerName(owner))
        {
            std::cout << "姓名无效：长度不能超过7，且只能包含字母和数字。\n";
            continue;
        }
        break;
    }

    Fen initBalanceCent = 0;
    while (true)
    {
        InputReadResult result = readAmountCent("[添加卡] 请输入初始余额（元)：", initBalanceCent);
        if (result == InputReadResult::Cancelled)
        {
            printCancelledAndBackToMenu();
            return;
        }
        if (result != InputReadResult::Success)
        {
            continue;
        }
        break;
    }

    std::string pin;
    while (true)
    {
        std::cout << "[添加卡] 请输入6位密码：";
        std::getline(std::cin, pin);
        if (isCancelInput(pin))
        {
            printCancelledAndBackToMenu();
            return;
        }
        if (!isValidPin(pin))
        {
            std::cout << "密码不合法,请检查再试\n";
            continue;
        }
        break;
    }

    try
    {
        g_cards.emplace_back(id, owner, pin, initBalanceCent);
        const Card& createdCard = g_cards.back();
        persistAllCardsToFile();
        std::cout << "--------添加成功--------\n";
        std::cout << "卡号\t姓名\t密码\t余额<元>\t开卡时间\n";
        std::cout << id << '\t' << owner << '\t' << pin << '\t' << formatCentToFen(initBalanceCent)
                  << " \t" << createdCard.getOpenTime() << '\n';
    }
    catch (const std::exception& e)
    {
        std::cout << "添加失败：" << e.what() << '\n';
    }

    waitEnter();
}

void CardService::queryCard()
{
    int id = 0;
    InputReadResult readResult = readInt("--------查询卡--------\n请输入卡号：", id);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    const Card* card = findCardByID(id);
    if (card == nullptr)
    {
        std::cout << "卡号不存在。\n";
        waitEnter();
        return;
    }

    std::cout << "卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n";
    std::cout << card->getID() << '\t'
              << statusToString(card->getStatus()) << '\t'
              << formatCentToFen(card->getBalanceCent()) << '\t'
              << card->getExpireTime() << '\t'
              << "0.0\t0\t" << card->getLastUseTime() << '\n';

    waitEnter();
}

void CardService::startSession()
{
    int id = 0;
    InputReadResult readResult = readInt("[上机] 请输入卡号：", id);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    Card* card = findCardByID(id);
    if (card == nullptr)
    {
        std::cout << "卡号不存在。\n";
        waitEnter();
        return;
    }

    if (card->getStatus() == CardStatus::OutofDate)
    {
        std::cout << "该卡已注销，无法上机。\n";
        waitEnter();
        return;
    }

    if (card->isInSession())
    {
        std::cout << "该卡已在上机中。\n";
        waitEnter();
        return;
    }

    card->setInSession(true);
    card->setStatus(CardStatus::Active);
    card->markLastUseNow();
    persistAllCardsToFile();
    std::cout << "上机成功。\n";
    waitEnter();
}

/*void CardService::endSession()
{
    int id = 0;
    InputReadResult readResult = readInt("[下机] 请输入卡号：", id);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    Card* card = findCardByID(id);
    if (card == nullptr)
    {
        std::cout << "卡号不存在。\n";
        waitEnter();
        return;
    }

    if (card->getStatus() == CardStatus::OutofDate)
    {
        std::cout << "该卡已注销，无法下机。\n";
        waitEnter();
        return;
    }

    if (!card->isInSession())
    {
        std::cout << "该卡当前不在上机状态。\n";
        waitEnter();
        return;
    }

    card->setInSession(false);
    card->setStatus(CardStatus::LoggedOut);
    card->markLastUseNow();
    persistAllCardsToFile();
    std::cout << "下机成功。\n";
    waitEnter();
}
*/
/*
void CardService::recharge()
{
    int id = 0;
    InputReadResult readResult = readInt("[充值] 请输入卡号：", id);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    Card* card = findCardByID(id);
    if (card == nullptr)
    {
        std::cout << "卡号不存在。\n";
        waitEnter();
        return;
    }

    if (card->getStatus() == CardStatus::OutofDate)
    {
        std::cout << "该卡已注销，无法充值。\n";
        waitEnter();
        return;
    }

    Fen amountCent = 0;
    readResult = readAmountCent("[充值] 请输入金额（元）：", amountCent);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    if (amountCent <= 0)
    {
        std::cout << "充值金额必须大于0。\n";
        waitEnter();
        return;
    }

    card->setBalanceCent(card->getBalanceCent() + amountCent);
    card->markLastUseNow();
    persistAllCardsToFile();
    std::cout << "充值成功，当前余额：" << formatCentToFen(card->getBalanceCent()) << " 元\n";
    waitEnter();
}

void CardService::refund()
{
    int id = 0;
    InputReadResult readResult = readInt("[退费] 请输入卡号：", id);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    Card* card = findCardByID(id);
    if (card == nullptr)
    {
        std::cout << "卡号不存在。\n";
        waitEnter();
        return;
    }

    if (card->getStatus() == CardStatus::OutofDate)
    {
        std::cout << "该卡已注销，无法退费。\n";
        waitEnter();
        return;
    }

    Fen amountCent = 0;
    readResult = readAmountCent("[退费] 请输入金额（元）：", amountCent);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    if (amountCent <= 0)
    {
        std::cout << "退费金额必须大于0。\n";
        waitEnter();
        return;
    }

    if (card->getBalanceCent() < amountCent)
    {
        std::cout << "余额不足，退费失败。\n";
        waitEnter();
        return;
    }

    card->setBalanceCent(card->getBalanceCent() - amountCent);
    card->markLastUseNow();
    persistAllCardsToFile();
    std::cout << "退费成功，当前余额：" << formatCentToFen(card->getBalanceCent()) << " 元\n";
    waitEnter();
}

void CardService::queryStats()
{
    std::cout << "[查询统计] 功能待实现\n";
    waitEnter();
}

void CardService::deleteCard()
{
    int id = 0;
    InputReadResult readResult = readInt("[注销卡] 请输入卡号：", id);
    if (readResult == InputReadResult::Cancelled)
    {
        printCancelledAndBackToMenu();
        return;
    }
    if (readResult != InputReadResult::Success)
    {
        waitEnter();
        return;
    }

    Card* card = findCardByID(id);
    if (card == nullptr)
    {
        std::cout << "卡号不存在。\n";
        waitEnter();
        return;
    }

    if (card->getStatus() == CardStatus::OutofDate)
    {
        std::cout << "该卡已注销，无需重复操作。\n";
        waitEnter();
        return;
    }

    if (card->isInSession())
    {
        std::cout << "该卡正在上机，无法注销。\n";
        waitEnter();
        return;
    }

    card->setStatus(CardStatus::OutofDate);
    card->setExpireTime(getCurrentDateTimeString());
    persistAllCardsToFile();
    std::cout << "注销成功，卡号 " << card->getID() << " 截止时间：" << card->getExpireTime() << '\n';
    waitEnter();
}
*/