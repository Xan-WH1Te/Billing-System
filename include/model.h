#pragma once

#include <time.h>

typedef struct Card
{
    char aName[18];
    char aPwd[8];
    int nStatus;
    time_t tStart;
    time_t tEnd;
    float fTotalUse;
    time_t tLast;
    int nUseCount;
    float fBalance;
    int nDel;
} Card;

typedef struct Billing
{
    char aCardName[18];
    time_t tStart;
    time_t tEnd;
    float fAmount;
    int nStatus;
    int nDel;
} Billing;

typedef struct LogonInfo
{
    char aCardName[18];
    time_t tLogon;
    float fBalance;
} LogonInfo;

typedef struct SettleInfo
{
    char aCardName[18];
    time_t tStart;
    time_t tEnd;
    float fAmount;
    float fBalance;
} SettleInfo;

typedef struct Money
{
    char aCardName[18];
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

typedef struct BillingNode
{
    Billing data;
    struct BillingNode* next;
} BillingNode;
