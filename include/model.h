#pragma once

#include <global.h>
#include <time.h>

typedef struct Card
{
    char aName[CARD_NAME_LEN + 1];
    char aPwd[CARD_PWD_LEN + 1];
    int nStatus;
    time_t tStart;
    time_t tEnd;
    float fTotalUse;
    time_t tLast;
    int nUseCount;
    float fBalance;
    int nDel;
} Card;

typedef struct LogonInfo
{
    char aCardName[CARD_NAME_LEN + 1];
    time_t tLogon;
    float fBalance;
} LogonInfo;

typedef struct SettleInfo
{
    char aCardName[CARD_NAME_LEN + 1];
    time_t tStart;
    time_t tEnd;
    float fAmount;
    float fBalance;
} SettleInfo;

typedef struct Money
{
    char aCardName[CARD_NAME_LEN + 1];
    time_t tTime;
    int nStatus;
    float fMoney;
    int nDel;
} Money;

typedef struct CardNode
{
    Card data;
    struct CardNode* next;
} CardNode;
