#pragma once

#include <string>

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
    public:
        Card(int id, std::string owner, const std::string& pin, Fen initBalance = 0);
        int getID() const { return id_; };
        const std::string& getOwnerName() const { return ownerName_; }
        Fen getBalanceCent() const { return balanceCent_; }
        CardStatus getStatus() const { return status_; }
        bool isInSession() const { return inSession_; }
        bool verifyPin(const std::string& pin) const { return pin_ == pin; }
        void setBalanceCent(Fen balance) { balanceCent_ = balance; }
        void setStatus(CardStatus status) { status_ = status; }
        void setInSession(bool inSession) { inSession_ = inSession; }
    
};

class CardService
{
    public:
        static void addCard();
        static void queryCard();
        static void startSession();
        static void endSession();
        static void recharge();
        static void refund();
        static void queryStats();
        static void deleteCard();
};
