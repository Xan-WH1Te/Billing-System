#pragma once

#define CARD_FILE_PATH "card.txt"
#define MONEY_FILE_PATH "money.txt"
#define ADMIN_FILE_PATH "admin.txt"
#define ADMIN_AUDIT_FILE_PATH "admin_audit.txt"
#define BILLING_RULE_FILE_PATH "billing_rule.txt"

#define INPUT_BUFFER_SIZE 128
#define CARD_NAME_LEN 7
#define CARD_PWD_LEN 6

#define CARD_STATUS_OFFLINE 0
#define CARD_STATUS_ONLINE 1
#define CARD_STATUS_DELETED 2

#define CHARGE_RATE_PER_SECOND 0.01f

#define REGISTER_CARD 0
#define CHARGE_CARD 1
#define REFUND_CARD 2
#define CONSUME_CARD 3
#define DELETE_CARD 4
#define ADJUST_CARD 5
#define PHYSICAL_DELETE_CARD 6