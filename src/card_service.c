#include <card_service.h>
#include <card_file.h>
#include <global.h>
#include <tool.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 全局卡链表头指针，服务生命周期内常驻内存。 */
static CardNode* g_card_head = 0;

/* 判断用户输入是否为取消命令 q/Q（忽略空白）。 */
static int is_cancel_input(const char* text)
{
    char temp[16];
    size_t i = 0;
    size_t j = 0;

    /* 过滤所有空白后再比较，允许用户输入 " q " 等形式。 */
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

/* 统一的取消提示输出。 */
static void print_cancelled(void)
{
    printf("已取消，返回菜单。\n");
}

/* 按卡号在线性链表中查找节点。 */
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

/* 关键字匹配：当前仅在卡号字段中做子串匹配。 */
static int card_match_keyword(const Card* card, const char* kw)
{
    return strstr(card->aName, kw) != 0;
}

static const char* status_to_text(int n_status);

/* 打印查询结果的一行明细。 */
static void print_card_detail_row(const Card* card)
{
    char end_text[32];
    char last_text[32];

    time_to_string(card->tEnd, end_text, sizeof(end_text));
    time_to_string(card->tLast, last_text, sizeof(last_text));

    printf("%s\t%s\t%.2f\t%s\t%.1f\t%d\t%s\n",
           card->aName,
           status_to_text(card->nStatus),
           card->fBalance,
           end_text,
           card->fTotalUse,
           card->nUseCount,
           last_text);
}

/* 将卡记录追加到全局链表尾部。 */
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

    /* 单链表无尾指针，追加时需要先遍历到末尾。 */
    tail = g_card_head;
    while (tail->next != 0)
    {
        tail = tail->next;
    }
    tail->next = node;
    return 1;
}

/* 校验卡号：必须是 7 位数字。 */
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

/* 校验密码：必须是 6 位数字。 */
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

/* 将状态码映射为展示文本。 */
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

/* 通用行输入函数：打印提示并去除末尾换行。 */
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

/* 从文件加载卡数据到内存。 */
void card_service_load(void)
{
    /* 先释放旧数据，避免重复加载导致链表泄漏。 */
    card_service_free();
    g_card_head = card_file_load_all(CARD_FILE_PATH);
}

/* 将当前内存中的卡数据落盘。 */
static void card_service_save(void)
{
    if (!card_file_save_all(g_card_head, CARD_FILE_PATH))
    {
        printf("警告：卡信息保存到文件失败。\n");
    }
}

/* 释放全局卡链表内存。 */
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

/* 添加新卡：依次完成卡号、密码、初始余额输入与校验。 */
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
        /* 卡号必须合法且唯一。 */
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
        /* 密码必须为 6 位数字。 */
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
        /* 使用 strtof 做严格数值解析，拒绝空串和非数字尾随字符。 */
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
    /* 初始化新卡默认状态。 */
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

    /* 输出添加结果摘要。 */
    time_to_string(card.tStart, time_text, sizeof(time_text));
    printf("--------添加成功--------\n");
    printf("卡号\t密码\t余额<元>\t开卡时间\n");
    printf("%s\t%s\t%.2f \t%s\n", card.aName, card.aPwd, card.fBalance, time_text);
    wait_enter();
}

/* 查询卡：支持精确卡号查询与关键字模糊查询。 */
void card_service_query_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;
    int match_count = 0;

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

    printf("-------------------------------查询结果------------------------------\n");
    printf("卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n");

    if (is_valid_card_name(card_name))
    {
        /* 输入是标准卡号时走精确查询。 */
        node = find_card_node(card_name);
        if (node == 0 || node->data.nDel == 1)
        {
            printf("卡号不存在。\n");
            wait_enter();
            return;
        }

        print_card_detail_row(&node->data);
        wait_enter();
        return;
    }

    node = g_card_head;
    /* 非标准卡号输入按关键字遍历匹配。 */
    while (node != 0)
    {
        if (node->data.nDel == 0 && card_match_keyword(&node->data, card_name))
        {
            print_card_detail_row(&node->data);
            ++match_count;
        }
        node = node->next;
    }

    if (match_count == 0)
    {
        printf("未找到匹配记录。\n");
    }
    else
    {
        printf("共找到 %d 条匹配记录。\n", match_count);
    }

    wait_enter();
}

/* 上机：将卡状态切换为在线并记录最近上机时间。 */
void card_service_start_session(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    char pwd[INPUT_BUFFER_SIZE];
    char start_time_text[32];
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
    /* 依次校验：存在性、是否注销、是否已在线。 */
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

    if (!read_line("[上机] 请输入密码：", pwd, sizeof(pwd)))
    {
        print_cancelled();
        return;
    }
    if (is_cancel_input(pwd))
    {
        print_cancelled();
        return;
    }
    if (strcmp(node->data.aPwd, pwd) != 0)
    {
        printf("密码错误，上机失败。\n");
        wait_enter();
        return;
    }

    node->data.nStatus = CARD_STATUS_ONLINE;
    /* tLast 作为最近一次“开始上机”时间。 */
    node->data.tLast = time(0);
    card_service_save();
    time_to_string(node->data.tLast, start_time_text, sizeof(start_time_text));
    printf("上机成功。\n");
    printf("卡号：%s\n", node->data.aName);   
    printf("当前余额：%.2f 元\n", node->data.fBalance);
    printf("上机时间：%s\n", start_time_text);

    wait_enter();
}

void card_service_end_session(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    char end_time_text[32];
    char start_time_text[32];
    time_t end_time;
    double seconds_used;
    double hours_used;
    float amount;
    CardNode* node;

    if (!read_line("[下机] 请输入卡号：", card_name, sizeof(card_name)))
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
    /* 依次校验：存在性、是否注销、是否已下机。 */
    if (node == 0 || node->data.nDel == 1)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }
    if (node->data.nStatus == CARD_STATUS_DELETED)
    {
        printf("该卡已注销，无法下机。\n");
        wait_enter();
        return;
    }
    if (node->data.nStatus == CARD_STATUS_OFFLINE)
    {
        printf("该卡已下机。\n");
        wait_enter();
        return;
    }

    node->data.nStatus = CARD_STATUS_OFFLINE;
    end_time = time(0);
    seconds_used = difftime(end_time, node->data.tLast);
    hours_used = seconds_used / 3600.0;
    amount = (float)(ceil(hours_used) * 2.0); /* 费用计算：每小时 2 元，按实际使用时间计费。 */
    node->data.fTotalUse += hours_used;
    node->data.fBalance -= amount;
    node->data.nUseCount += 1;

    /* 计算本次使用时长和费用，更新累计使用和余额。 */
    card_service_save();

    time_to_string(node->data.tLast, start_time_text, sizeof(start_time_text));
    time_to_string(end_time, end_time_text, sizeof(end_time_text));

    printf("-------------下机成功-------------\n");
    printf("卡号\t上机时间\t\t下机时间\t\t本次时长<小时>\t本次消费<元>\t余额<元>\n");
    printf("%s\t%s\t%s\t%.2f\t\t%.2f\t\t%.2f\n",
           node->data.aName,
           start_time_text,
           end_time_text,
           hours_used,
           amount,
           node->data.fBalance);

    if (node->data.fBalance < 0.0f)
    {
        printf("警告：余额不足，已欠费 %.2f 元，请尽快充值。\n", -node->data.fBalance);
    }
    wait_enter();
}

/* 充值功能占位。 */
void card_service_recharge(void)
{
    printf("[充值] 功能待实现\n");
    wait_enter();
}

/* 退费功能占位。 */
void card_service_refund(void)
{
    printf("[退费] 功能待实现\n");
    wait_enter();
}

/* 统计查询功能占位。 */
void card_service_query_stats(void)
{
    printf("[查询统计] 功能待实现\n");
    wait_enter();
}

/* 注销卡：仅允许离线卡注销，并记录截止时间。 */
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
    /* nStatus 负责业务状态，nDel 作为持久化删除标记。 */
    node->data.nDel = 1;
    node->data.tEnd = time(0);
    card_service_save();

    time_to_string(node->data.tEnd, end_time, sizeof(end_time));
    printf("注销成功，卡号 %s 截止时间：%s\n", node->data.aName, end_time);
    wait_enter();
}
