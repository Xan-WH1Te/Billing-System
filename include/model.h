#pragma once

#include <global.h>
#include <stdint.h>
#include <time.h>

typedef int64_t MoneyCent;

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
    MoneyCent nBalanceCent;
    int nDel;
} Card;

typedef struct LogonInfo
{
    char aCardName[CARD_NAME_LEN + 1];
    time_t tLogon;
    MoneyCent nBalanceCent;
} LogonInfo;

typedef struct SettleInfo
{
    char aCardName[CARD_NAME_LEN + 1];
    time_t tStart;
    time_t tEnd;
    MoneyCent nAmountCent;
    MoneyCent nBalanceCent;
} SettleInfo;

typedef struct Money
{
    char aCardName[CARD_NAME_LEN + 1];
    time_t tTime;
    int nStatus;
    MoneyCent nMoneyCent;
    MoneyCent nBeforeBalanceCent;
    MoneyCent nAfterBalanceCent;
    int nDel;
} Money;

typedef struct CardNode
{
    Card data;
    struct CardNode* next;
} CardNode;
