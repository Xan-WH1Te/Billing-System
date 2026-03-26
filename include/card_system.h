#pragma once

#include <stdbool.h>

typedef long long Fen;

#define OWNER_NAME_MAX_LEN 7
#define PIN_MAX_LEN 6
#define TIME_TEXT_MAX_LEN 31

typedef enum CardStatus
{
    Active,
    LoggedOut,
    OutofDate
} CardStatus;

typedef struct Card
{
    int id;
    char ownerName[OWNER_NAME_MAX_LEN + 1];
    Fen balanceCent;
    CardStatus status;
    int inSession;
    char pin[PIN_MAX_LEN + 1];
    char openTime[TIME_TEXT_MAX_LEN + 1];
    char expireTime[TIME_TEXT_MAX_LEN + 1];
    char lastUseTime[TIME_TEXT_MAX_LEN + 1];
} Card;

typedef struct CardNode
{
    Card card;
    struct CardNode* next;
} CardNode;

bool is_valid_pin(const char* pin);
void wait_enter(void);

void card_service_load_cards_from_file(void);
void card_service_add_card(void);
void card_service_query_card(void);
void card_service_start_session(void);
void card_service_end_session(void);
void card_service_recharge(void);
void card_service_refund(void);
void card_service_query_stats(void);
void card_service_delete_card(void);
void card_service_free_all(void);
