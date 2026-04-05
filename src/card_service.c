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

/* 统一输出业务分割标题。 */
static void print_service_header(const char* action_name)
{
    printf("--------%s--------\n", action_name);
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

/* 读取并校验卡号格式（7位数字）。 */
static int read_valid_card_name(const char* prompt, char* out, size_t out_size)
{
    while (1)
    {
        if (!read_line(prompt, out, out_size))
        {
            print_cancelled();
            return 0;
        }
        if (is_cancel_input(out))
        {
            print_cancelled();
            return 0;
        }
        if (!is_valid_card_name(out))
        {
            printf("卡号无效，请输入7位整数。\n");
            continue;
        }
        return 1;
    }
}

/* 读取并校验密码格式（6位数字）。 */
static int read_valid_password(const char* prompt, char* out, size_t out_size)
{
    while (1)
    {
        if (!read_line(prompt, out, out_size))
        {
            print_cancelled();
            return 0;
        }
        if (is_cancel_input(out))
        {
            print_cancelled();
            return 0;
        }
        if (!is_valid_pwd(out))
        {
            printf("密码不合法,请检查再试\n");
            continue;
        }
        return 1;
    }
}

/* 严格解析浮点输入，要求整行都是数字格式。 */
static int try_parse_float(const char* text, float* out_value)
{
    char* endptr;
    float parsed = strtof(text, &endptr);

    if (text[0] == '\0' || *endptr != '\0')
    {
        return 0;
    }

    *out_value = parsed;
    return 1;
}

/* 读取并校验金额，支持设置最小值及是否允许等于最小值。 */
static int read_amount_with_rule(const char* prompt,
                                 float* out_amount,
                                 float min_value,
                                 int allow_equal_min)
{
    char amount_text[INPUT_BUFFER_SIZE];
    float amount;

    while (1)
    {
        if (!read_line(prompt, amount_text, sizeof(amount_text)))
        {
            print_cancelled();
            return 0;
        }
        if (is_cancel_input(amount_text))
        {
            print_cancelled();
            return 0;
        }

        if (!try_parse_float(amount_text, &amount))
        {
            printf("金额输入无效。\n");
            continue;
        }

        if (allow_equal_min)
        {
            if (amount < min_value)
            {
                printf("金额输入无效。\n");
                continue;
            }
        }
        else
        {
            if (amount <= min_value)
            {
                printf("金额输入无效。\n");
                continue;
            }
        }

        *out_amount = amount;
        return 1;
    }
}

/* 按卡号获取存在的卡（未被逻辑删除）。 */
static int get_existing_card_by_name(const char* card_name, CardNode** out_node)
{
    CardNode* node = find_card_node(card_name);

    if (node == 0 || node->data.nDel == 1)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return 0;
    }

    *out_node = node;
    return 1;
}

/* 校验卡状态是否允许执行某个业务动作。 */
static int ensure_card_not_deleted_for_action(const CardNode* node, const char* action_name)
{
    if (node->data.nStatus == CARD_STATUS_DELETED)
    {
        printf("该卡已注销，无法%s。\n", action_name);
        wait_enter();
        return 0;
    }

    return 1;
}

/* 校验卡状态是否等于期望状态，不满足时输出业务提示。 */
static int ensure_card_status_for_action(const CardNode* node,
                                         int expected_status,
                                         const char* fail_message)
{
    if (node->data.nStatus != expected_status)
    {
        printf("%s\n", fail_message);
        wait_enter();
        return 0;
    }

    return 1;
}

/* 组合校验：先确保卡存在，再检查状态是否可用。 */
static int get_usable_card_for_action(const char* card_name,
                                      const char* action_name,
                                      CardNode** out_node)
{
    if (!get_existing_card_by_name(card_name, out_node))
    {
        return 0;
    }

    if (!ensure_card_not_deleted_for_action(*out_node, action_name))
    {
        return 0;
    }

    return 1;
}

/* 读取密码并校验是否与卡内密码一致。 */
static int verify_card_password(const CardNode* node,
                                const char* prompt,
                                const char* fail_message)
{
    char pwd[INPUT_BUFFER_SIZE];

    if (!read_valid_password(prompt, pwd, sizeof(pwd)))
    {
        return 0;
    }

    if (strcmp(node->data.aPwd, pwd) != 0)
    {
        printf("%s\n", fail_message);
        wait_enter();
        return 0;
    }

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
    Card card;
    float init_balance = 0.0f;
    char time_text[32];

    print_service_header("添加卡");

    while (1)
    {
        /* 卡号必须合法且唯一。 */
        if (!read_valid_card_name("[添加卡] 请输入卡号<7位整数>：", card_name, sizeof(card_name)))
        {
            return;
        }
        if (find_card_node(card_name) != 0)
        {
            printf("添加失败：卡号已存在。\n");
            continue;
        }
        break;
    }

    if (!read_valid_password("[添加卡] 请输入6位密码：", pwd, sizeof(pwd)))
    {
        return;
    }

    if (!read_amount_with_rule("[添加卡] 请输入初始余额（元)：", &init_balance, 0.0f, 1))
    {
        return;
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
    printf("-------------添加成功-------------\n");
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

    print_service_header("查询卡");

    if (!read_line("请输入卡号：", card_name, sizeof(card_name)))
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
        if (!get_existing_card_by_name(card_name, &node))
        {
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
    char start_time_text[32];
    CardNode* node;

    print_service_header("上机");

    if (!read_valid_card_name("[上机] 请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }

    if (!get_usable_card_for_action(card_name, "上机", &node))
    {
        return;
    }
    if (!ensure_card_status_for_action(node, CARD_STATUS_OFFLINE, "该卡已在上机中。"))
    {
        return;
    }

    if (!verify_card_password(node, "[上机] 请输入密码：", "密码错误，上机失败。"))
    {
        return;
    }

    node->data.nStatus = CARD_STATUS_ONLINE;
    /* tLast 作为最近一次“开始上机”时间。 */
    node->data.tLast = time(0);
    card_service_save();
    time_to_string(node->data.tLast, start_time_text, sizeof(start_time_text));
    printf("-------------上机成功-------------\n");
    printf("卡号\t余额<元>\t上机时间\n");
    printf("%s\t%.2f\t\t%s\n",
           node->data.aName,
           node->data.fBalance,
           start_time_text);

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

    print_service_header("下机");

    if (!read_valid_card_name("[下机] 请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }

    if (!get_usable_card_for_action(card_name, "下机", &node))
    {
        return;
    }
    if (!ensure_card_status_for_action(node, CARD_STATUS_ONLINE, "该卡已下机。"))
    {
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

/* 充值：校验卡号、状态、密码与金额后更新余额。 */
void card_service_recharge(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    char recharge_time_text[32];
    float amount;
    float old_balance;
    CardNode* node;

    print_service_header("充值");

    if (!read_valid_card_name("[充值] 请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }

    if (!get_usable_card_for_action(card_name, "充值", &node))
    {
        return;
    }

    if (!verify_card_password(node, "[充值] 请输入密码：", "密码错误，充值失败。"))
    {
        return;
    }

    if (!read_amount_with_rule("[充值] 请输入充值金额（元)：", &amount, 0.0f, 0))
    {
        return;
    }

    old_balance = node->data.fBalance;
    node->data.fBalance += amount;
    node->data.tLast = time(0);
    card_service_save();
        time_to_string(node->data.tLast, recharge_time_text, sizeof(recharge_time_text));
        printf("-------------充值成功-------------\n");
        printf("卡号\t充值金额<元>\t充值前余额<元>\t充值后余额<元>\t充值时间\n");
        printf("%s\t%.2f\t\t%.2f\t\t%.2f\t\t%s\n",
            node->data.aName,
            amount,
            old_balance,
            node->data.fBalance,
            recharge_time_text);
    wait_enter();
}

void card_service_refund(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    char refund_time_text[32];
    float amount;
    float old_balance;
    CardNode* node;
    print_service_header("退费");
    if (!read_valid_card_name("[退费] 请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }
    if (!get_usable_card_for_action(card_name, "退费", &node))
    {
        return;
    }
    if (!verify_card_password(node, "[退费] 请输入密码：", "密码错误，退费失败。"))
    {
        return;
    }
    printf("当前余额：%.2f 元\n", node->data.fBalance);
    while (1)
    {
        if (!read_amount_with_rule("[退费] 请输入退费金额（元)：", &amount, 0.0f, 0))
        {
            return;
        }
        if (amount > node->data.fBalance)
        {
            printf("退费金额不能大于当前余额。\n");
            wait_enter();
            continue;
        }
        break;
    }

    old_balance = node->data.fBalance;
    node->data.fBalance -= amount;
    node->data.tLast = time(0);
    card_service_save();
    time_to_string(node->data.tLast, refund_time_text, sizeof(refund_time_text));
    printf("-------------退费成功-------------\n");
    printf("卡号\t退费金额<元>\t退费前余额<元>\t退费后余额<元>\t退费时间\n");
    printf("%s\t%.2f\t\t%.2f\t\t%.2f\t\t%s\n",
        node->data.aName,
        amount,
        old_balance,
        node->data.fBalance,
        refund_time_text);
    wait_enter();
}

void card_service_query_stats(void)
{
    print_service_header("查询统计");

    wait_enter();
}

/* 注销卡：仅允许离线卡注销，并记录截止时间。 */
void card_service_delete_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;
    char end_time[32];
    char refund_time_text[32];
    float refund_amount = 0.0f;

    print_service_header("注销卡");

    if (!read_valid_card_name("[注销卡] 请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }
    if (!get_usable_card_for_action(card_name, "注销", &node))
    {
        return;
    }
    if (!ensure_card_status_for_action(node, CARD_STATUS_OFFLINE, "该卡正在上机，无法注销。"))
    {
        return;
    }
    if (!verify_card_password(node, "[注销卡] 请输入密码：", "密码错误，注销失败。"))
    {
        return;
    }

    /* 自动退所有费：注销时将正余额一次性退完并清零。 */
    if (node->data.fBalance > 0.0f)
    {
        refund_amount = node->data.fBalance;
        node->data.fBalance = 0.0f;
        node->data.tLast = time(0);
    }

    node->data.nStatus = CARD_STATUS_DELETED;
    /* nStatus 负责业务状态，nDel 作为持久化删除标记。 */
    node->data.nDel = 1;
    node->data.tEnd = time(0);
    card_service_save();

    if (refund_amount > 0.0f)
    {
        time_to_string(node->data.tLast, refund_time_text, sizeof(refund_time_text));
        printf("-------------自动退费成功-------------\n");
        printf("卡号\t退费金额<元>\t退费后余额<元>\t退费时间\n");
        printf("%s\t%.2f\t\t%.2f\t\t%s\n",
               node->data.aName,
               refund_amount,
               node->data.fBalance,
               refund_time_text);
    }
    else if (node->data.fBalance < 0.0f)
    {
        printf("当前余额为 %.2f 元（欠费），本次无可退金额。\n", node->data.fBalance);
    }
    else
    {
        printf("当前余额为0.00元，本次无可退金额。\n");
    }

    time_to_string(node->data.tEnd, end_time, sizeof(end_time));
    printf("-------------注销成功-------------\n");
    printf("卡号\t状态\t截止时间\t\t余额<元>\t退费金额<元>\n");
    printf("%s\t%s\t%s\t%.2f\t\t%.2f\n",
           node->data.aName,
           status_to_text(node->data.nStatus),
           end_time,
           node->data.fBalance,
           refund_amount);
    wait_enter();
}
