#pragma once

#include <string>
#include <utility>

using Fen = long long;

enum class CardStatus
{
    Active,
    LoggedOut,
    OutofDate
};

enum class TxType {
    Recharge,
    Refund,
    StartSession, 
    EndSession
};

bool isValidPin(const std::string& pin);

void waitEnter();

struct TxRecord
{
    int cardID{};
    Fen amount{};
    TxType type{};
    
};

class Card
{
    private:
        int id_;
        std::string ownerName_;
        Fen balanceCent_{0};
        CardStatus status_{CardStatus::Active};
        bool inSession_{false};
        std::string pin_;
        std::string openTime_;
        std::string expireTime_{"永久"};
        std::string lastUseTime_{"-"};
    public:
        Card(int id, std::string owner, const std::string& pin, Fen initBalance = 0);
        
        int getID() const { return id_; };
        const std::string& getOwnerName() const { return ownerName_; }
        Fen getBalanceCent() const { return balanceCent_; }
        CardStatus getStatus() const { return status_; }
        bool isInSession() const { return inSession_; }
        const std::string& getOpenTime() const { return openTime_; }
        const std::string& getExpireTime() const { return expireTime_; }
        const std::string& getLastUseTime() const { return lastUseTime_; }
        const std::string& getPin() const { return pin_; }
        bool verifyPin(const std::string& pin) const { return pin_ == pin; }
        void setBalanceCent(Fen balance) { balanceCent_ = balance; }
        void setStatus(CardStatus status) { status_ = status; }
        void setInSession(bool inSession) { inSession_ = inSession; }
        void setOpenTime(std::string openTime) { openTime_ = std::move(openTime); }
        void setExpireTime(std::string expireTime) { expireTime_ = std::move(expireTime); }
        void setLastUseTime(std::string lastUseTime) { lastUseTime_ = std::move(lastUseTime); }
        void markLastUseNow();
    
};

class CardService
{
    public:
        static void loadCardsFromFile();
        static void addCard();
        static void queryCard();
        static void startSession();
        static void endSession();
        static void recharge();
        static void refund();
        static void queryStats();
        static void deleteCard();
};
