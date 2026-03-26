#include <card_system.h>
#include <card_file.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static CardNode* g_cards = NULL;

static void trim_newline(char* text)
{
    size_t len;

    if (text == NULL)
    {
        return;
    }

    len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r'))
    {
        text[len - 1] = '\0';
        --len;
    }
}

static void trim_text(const char* text, char* out, size_t out_size)
{
    size_t left = 0;
    size_t right;
    size_t i = 0;

    if (text == NULL || out == NULL || out_size == 0)
    {
        return;
    }

    right = strlen(text);

    while (left < right && isspace((unsigned char)text[left]) != 0)
    {
        ++left;
    }

    while (right > left && isspace((unsigned char)text[right - 1]) != 0)
    {
        --right;
    }

    while (left < right && i + 1 < out_size)
    {
        out[i++] = text[left++];
    }

    out[i] = '\0';
}

static bool is_cancel_input(const char* input)
{
    char text[64];

    trim_text(input, text, sizeof(text));
    return strlen(text) == 1 && (text[0] == 'q' || text[0] == 'Q');
}

static void print_cancelled_and_back_to_menu(void)
{
    printf("已取消，返回菜单。\n");
}

bool is_valid_pin(const char* pin)
{
    size_t i;
    size_t length;

    if (pin == NULL)
    {
        return false;
    }

    length = strlen(pin);
    if (length != PIN_MAX_LEN)
    {
        return false;
    }

    for (i = 0; i < length; ++i)
    {
        if (isdigit((unsigned char)pin[i]) == 0)
        {
            return false;
        }
    }

    for (i = 0; i + 2 < length; ++i)
    {
        if (pin[i] == pin[i + 1] && pin[i + 1] == pin[i + 2])
        {
            return false;
        }
    }

    return true;
}

static bool is_valid_owner_name(const char* owner)
{
    size_t i;
    size_t length;

    if (owner == NULL)
    {
        return false;
    }

    length = strlen(owner);
    if (length == 0 || length > OWNER_NAME_MAX_LEN)
    {
        return false;
    }

    for (i = 0; i < length; ++i)
    {
        if (isalnum((unsigned char)owner[i]) == 0)
        {
            return false;
        }
    }

    return true;
}

static void get_current_datetime_string(char* out, size_t out_size)
{
    time_t now = time(NULL);
    struct tm* tm_ptr = localtime(&now);

    if (tm_ptr == NULL || out == NULL || out_size == 0)
    {
        return;
    }

    strftime(out, out_size, "%Y-%m-%d %H:%M:%S", tm_ptr);
}

static void init_card(Card* card, int id, const char* owner, const char* pin, Fen init_balance)
{
    if (card == NULL)
    {
        return;
    }

    card->id = id;
    strncpy(card->ownerName, owner, OWNER_NAME_MAX_LEN);
    card->ownerName[OWNER_NAME_MAX_LEN] = '\0';

    strncpy(card->pin, pin, PIN_MAX_LEN);
    card->pin[PIN_MAX_LEN] = '\0';

    card->balanceCent = init_balance;
    card->status = Active;
    card->inSession = 0;

    get_current_datetime_string(card->openTime, sizeof(card->openTime));
    strncpy(card->expireTime, "永久", sizeof(card->expireTime) - 1);
    card->expireTime[sizeof(card->expireTime) - 1] = '\0';

    strncpy(card->lastUseTime, "-", sizeof(card->lastUseTime) - 1);
    card->lastUseTime[sizeof(card->lastUseTime) - 1] = '\0';
}

static void mark_last_use_now(Card* card)
{
    if (card == NULL)
    {
        return;
    }

    get_current_datetime_string(card->lastUseTime, sizeof(card->lastUseTime));
}

void wait_enter(void)
{
    int ch;

    printf("按回车键继续...");
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

static CardNode* find_card_node_by_id(int id)
{
    CardNode* current = g_cards;

    while (current != NULL)
    {
        if (current->card.id == id)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

static bool card_exists(int id)
{
    return find_card_node_by_id(id) != NULL;
}

static const char* status_to_string(CardStatus status)
{
    switch (status)
    {
    case Active:
        return "Active";
    case LoggedOut:
        return "LoggedOut";
    case OutofDate:
        return "OutofDate";
    default:
        return "UnknownStatus";
    }
}

static void format_cent_to_fen(Fen cent, char* out, size_t out_size)
{
    snprintf(out, out_size, "%.2f", (double)cent / 100.0);
}

static bool read_int(const char* prompt, int* out_value)
{
    char line[128];
    char* endptr;
    long value;

    printf("%s", prompt);
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        printf("输入无效，请输入纯数字。\n");
        return false;
    }

    trim_newline(line);
    if (is_cancel_input(line))
    {
        return false;
    }

    value = strtol(line, &endptr, 10);
    if (*line == '\0' || *endptr != '\0')
    {
        printf("输入无效，请输入纯数字。\n");
        return false;
    }

    if (value > 2147483647L || value < -2147483647L)
    {
        printf("输入超出范围，请输入有效整数。\n");
        return false;
    }

    *out_value = (int)value;
    return true;
}

static bool read_amount_cent(const char* prompt, Fen* out_cent)
{
    char line[128];
    char* endptr;
    double amount_input;

    printf("%s", prompt);
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        printf("金额输入无效。\n");
        return false;
    }

    trim_newline(line);
    if (is_cancel_input(line))
    {
        return false;
    }

    amount_input = strtod(line, &endptr);
    if (*line == '\0' || *endptr != '\0')
    {
        printf("金额输入无效。\n");
        return false;
    }

    if (amount_input < 0)
    {
        printf("金额不能为负。\n");
        return false;
    }

    *out_cent = (Fen)llround(amount_input * 100.0);
    return true;
}

static void persist_all_cards_to_file(void)
{
    if (!save_all_cards(g_cards, "card.txt"))
    {
        printf("警告：卡信息保存到文件失败。\n");
    }
}

void card_service_load_cards_from_file(void)
{
    card_service_free_all();
    g_cards = read_cards("card.txt");
}

void card_service_add_card(void)
{
    int id = 0;
    char owner[64];
    Fen init_balance_cent = 0;
    char pin[64];
    Card card;
    CardNode* node;
    char balance_text[32];

    while (1)
    {
        if (!read_int("[添加卡] 请输入卡号<7位整数>：", &id))
        {
            print_cancelled_and_back_to_menu();
            return;
        }

        if (id > 9999999 || id <= 999999)
        {
            printf("卡号无效，请输入7位整数。\n");
            continue;
        }

        if (card_exists(id))
        {
            printf("添加失败：卡号已存在。\n");
            continue;
        }

        break;
    }

    while (1)
    {
        printf("[添加卡] 请输入姓名<长度不能超过7>：");
        if (fgets(owner, sizeof(owner), stdin) == NULL)
        {
            print_cancelled_and_back_to_menu();
            return;
        }

        trim_newline(owner);
        if (is_cancel_input(owner))
        {
            print_cancelled_and_back_to_menu();
            return;
        }

        if (!is_valid_owner_name(owner))
        {
            printf("姓名无效：长度不能超过7，且只能包含字母和数字。\n");
            continue;
        }

        break;
    }

    while (1)
    {
        if (!read_amount_cent("[添加卡] 请输入初始余额（元)：", &init_balance_cent))
        {
            print_cancelled_and_back_to_menu();
            return;
        }
        break;
    }

    while (1)
    {
        printf("[添加卡] 请输入6位密码：");
        if (fgets(pin, sizeof(pin), stdin) == NULL)
        {
            print_cancelled_and_back_to_menu();
            return;
        }

        trim_newline(pin);
        if (is_cancel_input(pin))
        {
            print_cancelled_and_back_to_menu();
            return;
        }

        if (!is_valid_pin(pin))
        {
            printf("密码不合法,请检查再试\n");
            continue;
        }

        break;
    }

    init_card(&card, id, owner, pin, init_balance_cent);

    node = (CardNode*)malloc(sizeof(CardNode));
    if (node == NULL)
    {
        printf("添加失败：内存不足。\n");
        wait_enter();
        return;
    }

    node->card = card;
    node->next = NULL;

    if (g_cards == NULL)
    {
        g_cards = node;
    }
    else
    {
        CardNode* tail = g_cards;
        while (tail->next != NULL)
        {
            tail = tail->next;
        }
        tail->next = node;
    }

    persist_all_cards_to_file();

    format_cent_to_fen(init_balance_cent, balance_text, sizeof(balance_text));
    printf("--------添加成功--------\n");
    printf("卡号\t姓名\t密码\t余额<元>\t开卡时间\n");
    printf("%d\t%s\t%s\t%s \t%s\n", id, owner, pin, balance_text, node->card.openTime);

    wait_enter();
}

void card_service_query_card(void)
{
    int id = 0;
    CardNode* node;
    char balance_text[32];

    if (!read_int("--------查询卡--------\n请输入卡号：", &id))
    {
        print_cancelled_and_back_to_menu();
        return;
    }

    node = find_card_node_by_id(id);
    if (node == NULL)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }

    format_cent_to_fen(node->card.balanceCent, balance_text, sizeof(balance_text));

    printf("卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n");
    printf("%d\t%s\t%s\t%s\t0.0\t0\t%s\n",
           node->card.id,
           status_to_string(node->card.status),
           balance_text,
           node->card.expireTime,
           node->card.lastUseTime);

    wait_enter();
}

void card_service_start_session(void)
{
    int id = 0;
    CardNode* node;

    if (!read_int("[上机] 请输入卡号：", &id))
    {
        print_cancelled_and_back_to_menu();
        return;
    }

    node = find_card_node_by_id(id);
    if (node == NULL)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }

    if (node->card.status == OutofDate)
    {
        printf("该卡已注销，无法上机。\n");
        wait_enter();
        return;
    }

    if (node->card.inSession)
    {
        printf("该卡已在上机中。\n");
        wait_enter();
        return;
    }

    node->card.inSession = 1;
    node->card.status = Active;
    mark_last_use_now(&node->card);
    persist_all_cards_to_file();
    printf("上机成功。\n");
    wait_enter();
}

void card_service_end_session(void)
{
    printf("[下机] 功能待实现\n");
    wait_enter();
}

void card_service_recharge(void)
{
    printf("[充值] 功能待实现\n");
    wait_enter();
}

void card_service_refund(void)
{
    printf("[退费] 功能待实现\n");
    wait_enter();
}

void card_service_query_stats(void)
{
    printf("[查询统计] 功能待实现\n");
    wait_enter();
}

void card_service_delete_card(void)
{
    printf("[注销卡] 功能待实现\n");
    wait_enter();
}

void card_service_free_all(void)
{
    CardNode* current = g_cards;

    while (current != NULL)
    {
        CardNode* next = current->next;
        free(current);
        current = next;
    }

    g_cards = NULL;
}
