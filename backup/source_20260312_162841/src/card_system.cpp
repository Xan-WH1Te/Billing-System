#include <card_system.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

static std::vector<Card> g_cards;

bool isValidPin(const std::string& pin)
{
    static const std::regex pattern(R"(^(?!(.*)(\d)\2\2)\d{6}$)");
    return std::regex_match(pin, pattern);
}

void waitEnter()
{
    std::cout << "按回车键继续...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

Card::Card(int id, std::string owner, const std::string& pin, Fen initBalance)
    : id_(id), ownerName_(std::move(owner)), balanceCent_(initBalance), pin_(pin)
{
    if (id <= 0) throw std::invalid_argument("卡号必须为正整数");
    if (ownerName_.empty()) throw std::invalid_argument("姓名不能为空");
    if (initBalance < 0) throw std::invalid_argument("初始余额不能为负");
    if (!isValidPin(pin_)) throw std::invalid_argument("密码不合法,请检查再试");
}

static void clearInputLine()
{
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static bool readInt(const std::string& prompt, int& outValue)
{
    std::cout << prompt;
    if (!(std::cin >> outValue))
    {
        std::cin.clear();
        clearInputLine();
        std::cout << "输入无效，请输入整数。\n";
        return false;
    }
    clearInputLine();
    return true;
}

static bool readAmountCent(const std::string& prompt, Fen& outCent)
{
    std::cout << prompt;
    double amountInput = 0.0;
    if (!(std::cin >> amountInput))
    {
        std::cin.clear();
        clearInputLine();
        std::cout << "金额输入无效。\n";
        return false;
    }
    clearInputLine();

    if (amountInput < 0)
    {
        std::cout << "金额不能为负。\n";
        return false;
    }

    outCent = static_cast<Fen>(std::llround(amountInput * 100.0));
    return true;
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
        return "Unknown";
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
    if (!readInt("[添加卡] 请输入卡号：", id))
    {
        waitEnter();
        return;
    }
    if (id <= 0)
    {
        std::cout << "卡号必须为正整数。\n";
        waitEnter();
        return;
    }

    if (cardExists(id))
    {
        std::cout << "添加失败：卡号已存在。\n";
        waitEnter();
        return;
    }

    std::string owner;
    std::cout << "[添加卡] 请输入姓名：";
    std::getline(std::cin, owner);
    if (owner.empty())
    {
        std::cout << "姓名不能为空。\n";
        waitEnter();
        return;
    }

    Fen initBalanceCent = 0;
    if (!readAmountCent("[添加卡] 请输入初始余额（元）：", initBalanceCent))
    {
        waitEnter();
        return;
    }

    std::string pin;
    std::cout << "[添加卡] 请输入6位密码：";
    std::getline(std::cin, pin);

    try
    {
        g_cards.emplace_back(id, owner, pin, initBalanceCent);
        std::cout << "添加成功：卡号 " << id << "，初始余额 " << formatCentToFen(initBalanceCent) << " 元。\n";
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
    if (!readInt("[查询卡] 请输入卡号：", id))
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

    std::cout << "卡号：" << card->getID() << '\n'
              << "姓名：" << card->getOwnerName() << '\n'
              << "余额：" << formatCentToFen(card->getBalanceCent()) << " 元\n"
              << "状态：" << statusToString(card->getStatus()) << '\n'
              << "上机中：" << (card->isInSession() ? "是" : "否") << '\n';

    waitEnter();
}

void CardService::startSession()
{
    std::cout << "[上机] 功能待实现\n";
    waitEnter();
}

void CardService::endSession()
{
    std::cout << "[下机] 功能待实现\n";
    waitEnter();
}

void CardService::recharge()
{
    int id = 0;
    if (!readInt("[充值] 请输入卡号：", id))
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

    Fen amountCent = 0;
    if (!readAmountCent("[充值] 请输入金额（元）：", amountCent))
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
    std::cout << "充值成功，当前余额：" << formatCentToFen(card->getBalanceCent()) << " 元\n";
    waitEnter();
}

void CardService::refund()
{
    int id = 0;
    if (!readInt("[退费] 请输入卡号：", id))
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

    Fen amountCent = 0;
    if (!readAmountCent("[退费] 请输入金额（元）：", amountCent))
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
    std::cout << "退费成功，当前余额：" << formatCentToFen(card->getBalanceCent()) << " 元\n";
    waitEnter();
}

void CardService::queryStats()
{
    int activeCount = 0;
    int inSessionCount = 0;
    Fen totalBalanceCent = 0;

    for (const auto& card : g_cards)
    {
        if (card.getStatus() == CardStatus::Active) ++activeCount;
        if (card.isInSession()) ++inSessionCount;
        totalBalanceCent += card.getBalanceCent();
    }

    std::cout << "[统计结果]\n"
              << "总卡数：" << g_cards.size() << '\n'
              << "活跃卡数：" << activeCount << '\n'
              << "上机卡数：" << inSessionCount << '\n'
              << "总余额：" << formatCentToFen(totalBalanceCent) << " 元\n";

    waitEnter();
}

void CardService::deleteCard()
{
    std::cout << "[注销卡] 功能待实现\n";
    waitEnter();
}
