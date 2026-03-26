#include <card_service.h>
#include <card_file.h>
#include <global.h>
#include <tool.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static CardNode* g_card_head = 0;

static int is_cancel_input(const char* text)
{
    char temp[16];
    size_t i = 0;
    size_t j = 0;

    while (text[i] != '\0' && j + 1 < sizeof(temp))
    {
        if (!isspace((unsigned char)text[i]))
        {
            temp[j++] = text[i];
        }
        ++i;
    }
    temp[j] = '\0';

    return j == 1 && (temp[0] == 'q' || temp[0] == 'Q');
}

static void print_cancelled(void)
{
    printf("已取消，返回菜单。\n");
}

static CardNode* find_card_node(const char* card_name)
{
    CardNode* current = g_card_head;

    while (current != 0)
    {
        if (strcmp(current->data.aName, card_name) == 0)
        {
            return current;
        }
        current = current->next;
    }

    return 0;
}

static int append_card(const Card* card)
{
    CardNode* node = (CardNode*)malloc(sizeof(CardNode));
    CardNode* tail;

    if (node == 0)
    {
        return 0;
    }

    node->data = *card;
    node->next = 0;

    if (g_card_head == 0)
    {
        g_card_head = node;
        return 1;
    }

    tail = g_card_head;
    while (tail->next != 0)
    {
        tail = tail->next;
    }
    tail->next = node;
    return 1;
}

static int is_valid_card_name(const char* card_name)
{
    size_t length = strlen(card_name);
    size_t i;

    if (length != 7)
    {
        return 0;
    }

    for (i = 0; i < length; ++i)
    {
        if (!isdigit((unsigned char)card_name[i]))
        {
            return 0;
        }
    }

    return 1;
}

static int is_valid_pwd(const char* pwd)
{
    size_t i;

    if (strlen(pwd) != 6)
    {
        return 0;
    }

    for (i = 0; i < 6; ++i)
    {
        if (!isdigit((unsigned char)pwd[i]))
        {
            return 0;
        }
    }

    return 1;
}

static const char* status_to_text(int n_status)
{
    if (n_status == CARD_STATUS_OFFLINE)
    {
        return "LoggedOut";
    }
    if (n_status == CARD_STATUS_ONLINE)
    {
        return "Active";
    }
    if (n_status == CARD_STATUS_DELETED)
    {
        return "OutofDate";
    }
    return "UnknownStatus";
}

static int read_line(const char* prompt, char* out, size_t out_size)
{
    printf("%s", prompt);
    if (fgets(out, (int)out_size, stdin) == 0)
    {
        return 0;
    }
    trim_newline(out);
    return 1;
}

void card_service_load(void)
{
    card_service_free();
    g_card_head = card_file_load_all(CARD_FILE_PATH);
}

void card_service_save(void)
{
    if (!card_file_save_all(g_card_head, CARD_FILE_PATH))
    {
        printf("警告：卡信息保存到文件失败。\n");
    }
}

void card_service_free(void)
{
    CardNode* current = g_card_head;

    while (current != 0)
    {
        CardNode* next = current->next;
        free(current);
        current = next;
    }

    g_card_head = 0;
}

void card_service_add_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    char pwd[INPUT_BUFFER_SIZE];
    char balance_text[INPUT_BUFFER_SIZE];
    Card card;
    float init_balance = 0.0f;
    char time_text[32];

    while (1)
    {
        if (!read_line("[添加卡] 请输入卡号<7位整数>：", card_name, sizeof(card_name)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_input(card_name))
        {
            print_cancelled();
            return;
        }
        if (!is_valid_card_name(card_name))
        {
            printf("卡号无效，请输入7位整数。\n");
            continue;
        }
        if (find_card_node(card_name) != 0)
        {
            printf("添加失败：卡号已存在。\n");
            continue;
        }
        break;
    }

    while (1)
    {
        if (!read_line("[添加卡] 请输入6位密码：", pwd, sizeof(pwd)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_input(pwd))
        {
            print_cancelled();
            return;
        }
        if (!is_valid_pwd(pwd))
        {
            printf("密码不合法,请检查再试\n");
            continue;
        }
        break;
    }

    while (1)
    {
        char* endptr;
        if (!read_line("[添加卡] 请输入初始余额（元)：", balance_text, sizeof(balance_text)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_input(balance_text))
        {
            print_cancelled();
            return;
        }

        init_balance = strtof(balance_text, &endptr);
        if (balance_text[0] == '\0' || *endptr != '\0' || init_balance < 0.0f)
        {
            printf("金额输入无效。\n");
            continue;
        }
        break;
    }

    memset(&card, 0, sizeof(card));
    strncpy(card.aName, card_name, sizeof(card.aName) - 1);
    strncpy(card.aPwd, pwd, sizeof(card.aPwd) - 1);
    card.nStatus = CARD_STATUS_OFFLINE;
    card.tStart = time(0);
    card.tEnd = 0;
    card.fTotalUse = 0.0f;
    card.tLast = 0;
    card.nUseCount = 0;
    card.fBalance = init_balance;
    card.nDel = 0;

    if (!append_card(&card))
    {
        printf("添加失败：内存不足。\n");
        wait_enter();
        return;
    }

    card_service_save();

    time_to_string(card.tStart, time_text, sizeof(time_text));
    printf("--------添加成功--------\n");
    printf("卡号\t密码\t余额<元>\t开卡时间\n");
    printf("%s\t%s\t%.2f \t%s\n", card.aName, card.aPwd, card.fBalance, time_text);
    wait_enter();
}

void card_service_query_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;
    char end_text[32];
    char last_text[32];

    if (!read_line("--------查询卡--------\n请输入卡号：", card_name, sizeof(card_name)))
    {
        print_cancelled();
        return;
    }
    if (is_cancel_input(card_name))
    {
        print_cancelled();
        return;
    }

    node = find_card_node(card_name);
    if (node == 0 || node->data.nDel == 1)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }

    time_to_string(node->data.tEnd, end_text, sizeof(end_text));
    time_to_string(node->data.tLast, last_text, sizeof(last_text));

    printf("卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n");
    printf("%s\t%s\t%.2f\t%s\t%.1f\t%d\t%s\n",
           node->data.aName,
           status_to_text(node->data.nStatus),
           node->data.fBalance,
           end_text,
           node->data.fTotalUse,
           node->data.nUseCount,
           last_text);

    wait_enter();
}

void card_service_start_session(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;

    if (!read_line("[上机] 请输入卡号：", card_name, sizeof(card_name)))
    {
        print_cancelled();
        return;
    }
    if (is_cancel_input(card_name))
    {
        print_cancelled();
        return;
    }

    node = find_card_node(card_name);
    if (node == 0 || node->data.nDel == 1)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }
    if (node->data.nStatus == CARD_STATUS_DELETED)
    {
        printf("该卡已注销，无法上机。\n");
        wait_enter();
        return;
    }
    if (node->data.nStatus == CARD_STATUS_ONLINE)
    {
        printf("该卡已在上机中。\n");
        wait_enter();
        return;
    }

    node->data.nStatus = CARD_STATUS_ONLINE;
    node->data.tLast = time(0);
    card_service_save();
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
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;
    char end_time[32];

    if (!read_line("[注销卡] 请输入卡号：", card_name, sizeof(card_name)))
    {
        print_cancelled();
        return;
    }
    if (is_cancel_input(card_name))
    {
        print_cancelled();
        return;
    }

    node = find_card_node(card_name);
    if (node == 0 || node->data.nDel == 1)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }
    if (node->data.nStatus == CARD_STATUS_ONLINE)
    {
        printf("该卡正在上机，无法注销。\n");
        wait_enter();
        return;
    }

    node->data.nStatus = CARD_STATUS_DELETED;
    node->data.nDel = 1;
    node->data.tEnd = time(0);
    card_service_save();

    time_to_string(node->data.tEnd, end_time, sizeof(end_time));
    printf("注销成功，卡号 %s 截止时间：%s\n", node->data.aName, end_time);
    wait_enter();
}
