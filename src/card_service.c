#include <card_service.h>
#include <auth_service.h>
#include <card_file.h>
#include <global.h>
#include <money_file.h>
#include <tool.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_BILLING_THRESHOLD_SECONDS 10.0f
#define DEFAULT_BILLING_SHORT_RATE 0.05f
#define DEFAULT_BILLING_BASE_FEE 0.5f
#define DEFAULT_BILLING_LONG_RATE 0.01f

/* 全局卡链表头指针，服务生命周期内常驻内存。 */
static CardNode* g_card_head = 0;
/* 计费标准：阈值秒数、阈值内每秒单价、达到阈值基础费、超出后每秒单价。 */
static float g_billing_threshold_seconds = DEFAULT_BILLING_THRESHOLD_SECONDS;
static float g_billing_short_rate = DEFAULT_BILLING_SHORT_RATE;
static float g_billing_base_fee = DEFAULT_BILLING_BASE_FEE;
static float g_billing_long_rate = DEFAULT_BILLING_LONG_RATE;

static int billing_rule_is_valid(float threshold, float short_rate, float base_fee, float long_rate)
{
    return threshold > 0.0f && short_rate >= 0.0f && base_fee >= 0.0f && long_rate >= 0.0f;
}

static void save_billing_rule(void)
{
    FILE* file = fopen(BILLING_RULE_FILE_PATH, "w");

    if (file == 0)
    {
        printf("警告：计费标准保存失败。\n");
        return;
    }

    fprintf(file,
            "%.2f##%.4f##%.2f##%.4f\n",
            g_billing_threshold_seconds,
            g_billing_short_rate,
            g_billing_base_fee,
            g_billing_long_rate);
    fclose(file);
}

static void load_billing_rule(void)
{
    FILE* file = fopen(BILLING_RULE_FILE_PATH, "r");
    char line[128];
    float threshold;
    float short_rate;
    float base_fee;
    float long_rate;

    if (file == 0)
    {
        save_billing_rule();
        return;
    }

    if (fgets(line, sizeof(line), file) == 0)
    {
        fclose(file);
        return;
    }

    trim_newline(line);
    if (sscanf(line, "%f##%f##%f##%f", &threshold, &short_rate, &base_fee, &long_rate) == 4 &&
        billing_rule_is_valid(threshold, short_rate, base_fee, long_rate))
    {
        g_billing_threshold_seconds = threshold;
        g_billing_short_rate = short_rate;
        g_billing_base_fee = base_fee;
        g_billing_long_rate = long_rate;
    }

    fclose(file);
}

/* 统一的取消提示输出。 */
static void print_cancelled(void)
{
    printf("已取消，返回菜单。\n");
}

/* 统一输出业务分割标题。 */
static void print_service_header(const char* action_name)
{
    printf("    --------%s--------\n", action_name);
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
static int confirm_delete_card_two_steps(const char* card_name);

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
    return read_line_safely(prompt, out, out_size) ? 1 : 0;
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
        if (is_cancel_command(out))
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
        if (is_cancel_command(out))
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
    return parse_float_strict(text, out_value) ? 1 : 0;
}

/* 严格解析整数输入，要求整行都是整数格式。 */
static int try_parse_int(const char* text, int* out_value)
{
    return parse_int32_strict(text, out_value) ? 1 : 0;
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
        if (is_cancel_command(amount_text))
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

    if (node == 0)
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

    /* 管理员登录后，业务操作统一免密码。 */
    if (auth_is_admin())
    {
        (void)node;
        (void)prompt;
        (void)fail_message;
        return 1;
    }

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

/* 资金变动类型映射为展示文本。 */
static const char* money_status_to_text(int n_status)
{
    if (n_status == REGISTER_CARD)
    {
        return "注册";
    }
    if (n_status == CHARGE_CARD)
    {
        return "充值";
    }
    if (n_status == REFUND_CARD)
    {
        return "退费";
    }
    if (n_status == CONSUME_CARD)
    {
        return "消费";
    }
    if (n_status == DELETE_CARD)
    {
        return "注销退费";
    }
    if (n_status == PHYSICAL_DELETE_CARD)
    {
        return "删除退费";
    }
    if (n_status == ADJUST_CARD)
    {
        return "管理员调账";
    }
    return "未知";
}

/* 记录一条余额变动流水。 */
static void record_money_change(const char* card_name,
                                int status,
                                float amount,
                                float before_balance,
                                float after_balance,
                                time_t event_time)
{
    Money record;

    memset(&record, 0, sizeof(record));
    strncpy(record.aCardName, card_name, sizeof(record.aCardName) - 1);
    record.tTime = event_time;
    record.nStatus = status;
    record.fMoney = amount;
    record.fBeforeBalance = before_balance;
    record.fAfterBalance = after_balance;
    record.nDel = 0;

    if (!money_file_append(&record, MONEY_FILE_PATH))
    {
        printf("警告：资金流水保存失败。\n");
    }
}

/* 读取并校验时间输入，格式 YYYY-MM-DD HH:MM:SS。 */
static int read_valid_time_input(const char* prompt, time_t* out_time)
{
    char time_text[INPUT_BUFFER_SIZE];

    while (1)
    {
        if (!read_line(prompt, time_text, sizeof(time_text)))
        {
            print_cancelled();
            return 0;
        }
        if (is_cancel_command(time_text))
        {
            print_cancelled();
            return 0;
        }
        if (!string_to_time(time_text, out_time))
        {
            printf("时间格式无效，请输入 YYYY-MM-DD HH:MM:SS。\n");
            continue;
        }
        return 1;
    }
}

/* 读取并校验时间段，要求开始时间不晚于结束时间。 */
static int read_time_range(time_t* out_start, time_t* out_end)
{
    if (!read_valid_time_input("请输入开始时间(YYYY-MM-DD HH:MM:SS)：", out_start))
    {
        return 0;
    }
    if (!read_valid_time_input("请输入结束时间(YYYY-MM-DD HH:MM:SS)：", out_end))
    {
        return 0;
    }

    if (*out_start > *out_end)
    {
        printf("时间段无效：开始时间不能晚于结束时间。\n");
        wait_enter();
        return 0;
    }

    return 1;
}

/* 读取并校验年份。 */
static int read_valid_year(const char* prompt, int* out_year)
{
    char year_text[INPUT_BUFFER_SIZE];
    int year_value;

    while (1)
    {
        if (!read_line(prompt, year_text, sizeof(year_text)))
        {
            print_cancelled();
            return 0;
        }
        if (is_cancel_command(year_text))
        {
            print_cancelled();
            return 0;
        }

        if (!try_parse_int(year_text, &year_value) || year_value < 1970 || year_value > 3000)
        {
            printf("年份输入无效。\n");
            continue;
        }

        *out_year = year_value;
        return 1;
    }
}

/* 查询：按卡号和时间段查询消费记录。 */
static void query_consume_records_by_card_and_time(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    time_t start_time;
    time_t end_time;
    Money* records = 0;
    size_t count = 0;
    size_t i;
    int match_count = 0;

    if (!read_valid_card_name("请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }
    if (!read_time_range(&start_time, &end_time))
    {
        return;
    }

    if (!money_file_load_all(MONEY_FILE_PATH, &records, &count))
    {
        printf("读取资金流水失败。\n");
        wait_enter();
        return;
    }

    printf("-------------消费记录查询结果-------------\n");
    printf("卡号\t时间\t\t\t类型\t金额<元>\t前余额<元>\t后余额<元>\n");

    for (i = 0; i < count; ++i)
    {
        char time_text[32];
        Money* rec = &records[i];

        if (rec->nDel == 1)
        {
            continue;
        }
        if (rec->nStatus != CONSUME_CARD)
        {
            continue;
        }
        if (strcmp(rec->aCardName, card_name) != 0)
        {
            continue;
        }
        if (rec->tTime < start_time || rec->tTime > end_time)
        {
            continue;
        }

        time_to_string(rec->tTime, time_text, sizeof(time_text));
        printf("%s\t%s\t%s\t%.2f\t\t%.2f\t\t%.2f\n",
               rec->aCardName,
               time_text,
               money_status_to_text(rec->nStatus),
               rec->fMoney,
               rec->fBeforeBalance,
               rec->fAfterBalance);
        ++match_count;
    }

    if (match_count == 0)
    {
        printf("未找到消费记录。\n");
    }
    else
    {
        printf("共找到 %d 条消费记录。\n", match_count);
    }

    money_file_free_all(records);
    wait_enter();
}

/* 查询：按时间段统计总营业额（仅统计消费）。 */
static void query_total_revenue_by_time(void)
{
    time_t start_time;
    time_t end_time;
    Money* records = 0;
    size_t count = 0;
    size_t i;
    int consume_count = 0;
    float total_revenue = 0.0f;

    if (!read_time_range(&start_time, &end_time))
    {
        return;
    }

    if (!money_file_load_all(MONEY_FILE_PATH, &records, &count))
    {
        printf("读取资金流水失败。\n");
        wait_enter();
        return;
    }

    for (i = 0; i < count; ++i)
    {
        Money* rec = &records[i];

        if (rec->nDel == 1)
        {
            continue;
        }
        if (rec->nStatus != CONSUME_CARD)
        {
            continue;
        }
        if (rec->tTime < start_time || rec->tTime > end_time)
        {
            continue;
        }

        total_revenue += rec->fMoney;
        ++consume_count;
    }

    printf("-------------总营业额统计结果-------------\n");
    printf("统计口径：仅统计消费金额\n");
    printf("消费笔数：%d\n", consume_count);
    printf("总营业额：%.2f 元\n", total_revenue);

    money_file_free_all(records);
    wait_enter();
}

/* 查询：统计某年的每月营业额（仅统计消费）。 */
static void query_monthly_revenue_by_year(void)
{
    int year;
    Money* records = 0;
    size_t count = 0;
    size_t i;
    float monthly[12] = {0.0f};
    float yearly_total = 0.0f;

    if (!read_valid_year("请输入年份（如 2026）：", &year))
    {
        return;
    }

    if (!money_file_load_all(MONEY_FILE_PATH, &records, &count))
    {
        printf("读取资金流水失败。\n");
        wait_enter();
        return;
    }

    for (i = 0; i < count; ++i)
    {
        struct tm tm_value;
        int record_year;
        int month_index;
        Money* rec = &records[i];

        if (rec->nDel == 1)
        {
            continue;
        }
        if (rec->nStatus != CONSUME_CARD)
        {
            continue;
        }

        if (!localtime_safe(rec->tTime, &tm_value))
        {
            continue;
        }

        record_year = tm_value.tm_year + 1900;
        if (record_year != year)
        {
            continue;
        }

        month_index = tm_value.tm_mon;
        if (month_index < 0 || month_index > 11)
        {
            continue;
        }

        monthly[month_index] += rec->fMoney;
    }

    printf("-------------月营业额统计结果-------------\n");
    printf("年份：%d\n", year);
    printf("月份\t营业额<元>\n");
    for (i = 0; i < 12; ++i)
    {
        printf("%02d\t%.2f\n", (int)(i + 1), monthly[i]);
        yearly_total += monthly[i];
    }
    printf("全年\t%.2f\n", yearly_total);

    money_file_free_all(records);
    wait_enter();
}

/* 从文件加载卡数据到内存。 */
void card_service_load(void)
{
    /* 先释放旧数据，避免重复加载导致链表泄漏。 */
    card_service_free();
    g_card_head = card_file_load_all(CARD_FILE_PATH);
    load_billing_rule();
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
    char restore_text[INPUT_BUFFER_SIZE];
    Card card;
    CardNode* existing_node;
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
        existing_node = find_card_node(card_name);
        if (existing_node != 0)
        {
            if (existing_node->data.nStatus == CARD_STATUS_DELETED)
            {
                if (!read_line("检测到该卡已注销，是否恢复该卡？(YES/NO)：", restore_text, sizeof(restore_text)))
                {
                    print_cancelled();
                    return;
                }
                if (is_cancel_command(restore_text))
                {
                    print_cancelled();
                    return;
                }

                if (strcmp(restore_text, "YES") == 0 || strcmp(restore_text, "yes") == 0)
                {
                    if (!verify_card_password(existing_node,
                                              "[恢复卡] 请输入原密码：",
                                              "密码错误，恢复失败。"))
                    {
                        continue;
                    }

                    existing_node->data.nStatus = CARD_STATUS_OFFLINE;
                    existing_node->data.nDel = 0;
                    existing_node->data.tEnd = 0;
                    existing_node->data.tLast = time(0);
                    card_service_save();
                    printf("恢复成功：卡号 %s 已恢复为可用状态。\n", existing_node->data.aName);
                    wait_enter();
                    return;
                }

                printf("已取消恢复，请输入其他卡号继续添加。\n");
                continue;
            }

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
    record_money_change(card.aName,
                        REGISTER_CARD,
                        init_balance,
                        0.0f,
                        card.fBalance,
                        card.tStart);

    /* 输出添加结果摘要。 */
    time_to_string(card.tStart, time_text, sizeof(time_text));
    printf("-------------添加成功-------------\n");
    printf("卡号\t密码\t余额<元>\t开卡时间\n");
    printf("%s\t%s\t%.2f \t%s\n", card.aName, card.aPwd, card.fBalance, time_text);
    wait_enter();
}

/* 查询卡：管理员可精确/模糊查询；普通用户仅可凭卡号+密码查询本人。 */
void card_service_query_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    int is_admin = auth_is_admin() ? 1 : 0;

    print_service_header("查询卡");

    while (1)
    {
        CardNode* node;
        int match_count = 0;

        if (!read_line("请输入卡号（管理员支持关键字，q/Q 取消）：", card_name, sizeof(card_name)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_command(card_name))
        {
            print_cancelled();
            return;
        }

        if (!is_admin)
        {
            if (!is_valid_card_name(card_name))
            {
                printf("普通用户仅支持按7位卡号精确查询。\n");
                wait_enter();
                continue;
            }

            node = find_card_node(card_name);
            if (node == 0)
            {
                printf("卡号不存在。\n");
                wait_enter();
                continue;
            }

            if (!verify_card_password(node, "[查询卡] 请输入密码：", "密码错误，查询失败。"))
            {
                continue;
            }

            printf("-------------------------------查询结果------------------------------\n");
            printf("卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n");
            print_card_detail_row(&node->data);
            wait_enter();
            return;
        }

        printf("-------------------------------查询结果------------------------------\n");
        printf("卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n");

        if (is_valid_card_name(card_name))
        {
            node = find_card_node(card_name);
            if (node == 0)
            {
                printf("卡号不存在。\n");
                wait_enter();
                continue;
            }

            print_card_detail_row(&node->data);
            wait_enter();
            return;
        }

        node = g_card_head;
        while (node != 0)
        {
            if (card_match_keyword(&node->data, card_name))
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
        return;
    }
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
    float amount;
    float old_balance;
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

    if (!verify_card_password(node, "[下机] 请输入密码：", "密码错误，下机失败。"))
    {
        return;
    }

    node->data.nStatus = CARD_STATUS_OFFLINE;
    end_time = time(0);
    seconds_used = difftime(end_time, node->data.tLast);
    if (seconds_used < 0.0)
    {
        seconds_used = 0.0;
    }

    /* 动态计费：阈值内按每秒单价；达到阈值后按基础费+超出秒数单价。 */
    if (seconds_used < g_billing_threshold_seconds)
    {
        amount = (float)(seconds_used * g_billing_short_rate);
    }
    else
    {
        amount = (float)(g_billing_base_fee +
                         (seconds_used - g_billing_threshold_seconds) * g_billing_long_rate);
    }

    old_balance = node->data.fBalance;
    node->data.fTotalUse += seconds_used;
    node->data.fBalance -= amount;
    node->data.nUseCount += 1;

    /* 计算本次使用时长和费用，更新累计使用和余额。 */
    card_service_save();
    record_money_change(node->data.aName,
                        CONSUME_CARD,
                        amount,
                        old_balance,
                        node->data.fBalance,
                        end_time);

    time_to_string(node->data.tLast, start_time_text, sizeof(start_time_text));
    time_to_string(end_time, end_time_text, sizeof(end_time_text));

    printf("-------------下机成功-------------\n");
    printf("卡号\t上机时间\t\t下机时间\t\t本次时长<秒>\t本次消费<元>\t余额<元>\n");
    printf("%s\t%s\t%s\t%.2f\t\t%.2f\t\t%.2f\n",
           node->data.aName,
           start_time_text,
           end_time_text,
           seconds_used,
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
    float bonus = 0.0f;
    float credited_amount;
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

    printf("充值活动:满50送10，满100送25！！\n");

    if (!read_amount_with_rule("[充值] 请输入充值金额（元)：", &amount, 0.0f, 0))
    {
        return;
    }

    /* 动态充值赠送：充值>=100送25；否则充值>=50送10。 */
    if (amount >= 100.0f)
    {
        bonus = 25.0f;
    }
    else if (amount >= 50.0f)
    {
        bonus = 10.0f;
    }

    credited_amount = amount + bonus;

    old_balance = node->data.fBalance;
    node->data.fBalance += credited_amount;
    node->data.tLast = time(0);
    card_service_save();
    record_money_change(node->data.aName,
                        CHARGE_CARD,
                        credited_amount,
                        old_balance,
                        node->data.fBalance,
                        node->data.tLast);
    time_to_string(node->data.tLast, recharge_time_text, sizeof(recharge_time_text));
    printf("-------------充值成功-------------\n");
    printf("卡号\t实充金额<元>\t赠送金额<元>\t到账金额<元>\t充值前余额<元>\t充值后余额<元>\t充值时间\n");
    printf("%s\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f\t\t%.2f\t\t%s\n",
           node->data.aName,
           amount,
           bonus,
           credited_amount,
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

    if (!ensure_card_status_for_action(node, CARD_STATUS_OFFLINE, "该卡正在上机，无法退费。"))
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
            continue;
        }
        break;
    }

    old_balance = node->data.fBalance;
    node->data.fBalance -= amount;
    node->data.tLast = time(0);
    card_service_save();
    record_money_change(node->data.aName,
                        REFUND_CARD,
                        amount,
                        old_balance,
                        node->data.fBalance,
                        node->data.tLast);
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
    char choice_text[INPUT_BUFFER_SIZE];

    print_service_header("查询统计");

    while (1)
    {
        printf("1. 消费记录查询（卡号+时间段）\n");
        printf("2. 统计总营业额（时间段）\n");
        printf("3. 统计月营业额（按年份）\n");
        printf("0. 返回\n");
        if (!read_line("请选择菜单项：", choice_text, sizeof(choice_text)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_command(choice_text) || strcmp(choice_text, "0") == 0)
        {
            return;
        }

        if (strcmp(choice_text, "1") == 0)
        {
            query_consume_records_by_card_and_time();
            continue;
        }
        if (strcmp(choice_text, "2") == 0)
        {
            query_total_revenue_by_time();
            continue;
        }
        if (strcmp(choice_text, "3") == 0)
        {
            query_monthly_revenue_by_year();
            continue;
        }

        printf("输入无效，请输入 0~3。\n");
    }
}

void card_service_admin_manage_pricing(void)
{
    if (!auth_is_admin())
    {
        printf("请先进行管理员登录。\n");
        wait_enter();
        return;
    }

    while (1)
    {
        char choice_text[INPUT_BUFFER_SIZE];
        char value_text[INPUT_BUFFER_SIZE];
        char before_text[128];
        char after_text[128];
        int choice = -1;
        float new_value;

        print_service_header("计费标准设置");
        printf("当前阈值秒数：%.2f\n", g_billing_threshold_seconds);
        printf("当前阈值内单价：%.4f 元/秒\n", g_billing_short_rate);
        printf("当前阈值基础费：%.2f 元\n", g_billing_base_fee);
        printf("当前超出后单价：%.4f 元/秒\n", g_billing_long_rate);
        printf("计费说明：时长 < 阈值 => 秒数 * 阈值内单价\n");
        printf("          时长 >= 阈值 => 阈值基础费 + 超出秒数 * 超出后单价\n");
        printf("1. 修改阈值秒数\n");
        printf("2. 修改阈值内单价\n");
        printf("3. 修改阈值基础费\n");
        printf("4. 修改超出后单价\n");
        printf("5. 恢复默认计费标准\n");
        printf("0. 返回\n");

        if (!read_line("请选择菜单项：", choice_text, sizeof(choice_text)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_command(choice_text) || strcmp(choice_text, "0") == 0)
        {
            return;
        }
        if (!try_parse_int(choice_text, &choice))
        {
            printf("输入无效。\n");
            wait_enter();
            continue;
        }

        snprintf(before_text,
                 sizeof(before_text),
                 "T=%.2f,S=%.4f,B=%.2f,L=%.4f",
                 g_billing_threshold_seconds,
                 g_billing_short_rate,
                 g_billing_base_fee,
                 g_billing_long_rate);

        if (choice == 1)
        {
            if (!read_line("请输入新阈值秒数(>0)：", value_text, sizeof(value_text)))
            {
                print_cancelled();
                return;
            }
            if (is_cancel_command(value_text))
            {
                print_cancelled();
                return;
            }
            if (!try_parse_float(value_text, &new_value) || new_value <= 0.0f)
            {
                printf("阈值秒数输入无效。\n");
                wait_enter();
                continue;
            }

            g_billing_threshold_seconds = new_value;
            save_billing_rule();
            snprintf(after_text,
                     sizeof(after_text),
                     "T=%.2f,S=%.4f,B=%.2f,L=%.4f",
                     g_billing_threshold_seconds,
                     g_billing_short_rate,
                     g_billing_base_fee,
                     g_billing_long_rate);
            auth_append_audit("修改计费标准", "SYSTEM", before_text, after_text);
            printf("阈值秒数修改成功。\n");
            wait_enter();
            continue;
        }

        if (choice == 2)
        {
            if (!read_line("请输入新阈值内单价(>=0)：", value_text, sizeof(value_text)))
            {
                print_cancelled();
                return;
            }
            if (is_cancel_command(value_text))
            {
                print_cancelled();
                return;
            }
            if (!try_parse_float(value_text, &new_value) || new_value < 0.0f)
            {
                printf("阈值内单价输入无效。\n");
                wait_enter();
                continue;
            }

            g_billing_short_rate = new_value;
            save_billing_rule();
            snprintf(after_text,
                     sizeof(after_text),
                     "T=%.2f,S=%.4f,B=%.2f,L=%.4f",
                     g_billing_threshold_seconds,
                     g_billing_short_rate,
                     g_billing_base_fee,
                     g_billing_long_rate);
            auth_append_audit("修改计费标准", "SYSTEM", before_text, after_text);
            printf("阈值内单价修改成功。\n");
            wait_enter();
            continue;
        }

        if (choice == 3)
        {
            if (!read_line("请输入新阈值基础费(>=0)：", value_text, sizeof(value_text)))
            {
                print_cancelled();
                return;
            }
            if (is_cancel_command(value_text))
            {
                print_cancelled();
                return;
            }
            if (!try_parse_float(value_text, &new_value) || new_value < 0.0f)
            {
                printf("阈值基础费输入无效。\n");
                wait_enter();
                continue;
            }

            g_billing_base_fee = new_value;
            save_billing_rule();
            snprintf(after_text,
                     sizeof(after_text),
                     "T=%.2f,S=%.4f,B=%.2f,L=%.4f",
                     g_billing_threshold_seconds,
                     g_billing_short_rate,
                     g_billing_base_fee,
                     g_billing_long_rate);
            auth_append_audit("修改计费标准", "SYSTEM", before_text, after_text);
            printf("阈值基础费修改成功。\n");
            wait_enter();
            continue;
        }

        if (choice == 4)
        {
            if (!read_line("请输入新超出后单价(>=0)：", value_text, sizeof(value_text)))
            {
                print_cancelled();
                return;
            }
            if (is_cancel_command(value_text))
            {
                print_cancelled();
                return;
            }
            if (!try_parse_float(value_text, &new_value) || new_value < 0.0f)
            {
                printf("超出后单价输入无效。\n");
                wait_enter();
                continue;
            }

            g_billing_long_rate = new_value;
            save_billing_rule();
            snprintf(after_text,
                     sizeof(after_text),
                     "T=%.2f,S=%.4f,B=%.2f,L=%.4f",
                     g_billing_threshold_seconds,
                     g_billing_short_rate,
                     g_billing_base_fee,
                     g_billing_long_rate);
            auth_append_audit("修改计费标准", "SYSTEM", before_text, after_text);
            printf("超出后单价修改成功。\n");
            wait_enter();
            continue;
        }

        if (choice == 5)
        {
            g_billing_threshold_seconds = DEFAULT_BILLING_THRESHOLD_SECONDS;
            g_billing_short_rate = DEFAULT_BILLING_SHORT_RATE;
            g_billing_base_fee = DEFAULT_BILLING_BASE_FEE;
            g_billing_long_rate = DEFAULT_BILLING_LONG_RATE;
            save_billing_rule();
            snprintf(after_text,
                     sizeof(after_text),
                     "T=%.2f,S=%.4f,B=%.2f,L=%.4f",
                     g_billing_threshold_seconds,
                     g_billing_short_rate,
                     g_billing_base_fee,
                     g_billing_long_rate);
            auth_append_audit("重置计费标准", "SYSTEM", before_text, after_text);
            printf("已恢复默认计费标准。\n");
            wait_enter();
            continue;
        }

        printf("输入无效，请输入 0~5。\n");
        wait_enter();
    }
}

/* 管理员直改卡信息：允许直接修改关键字段并实时落盘。 */
void card_service_admin_edit_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;

    print_service_header("管理员修改卡信息");

    if (!auth_is_admin())
    {
        printf("请先进行管理员登录。\n");
        wait_enter();
        return;
    }

    if (!read_valid_card_name("[管理员修改] 请输入卡号：", card_name, sizeof(card_name)))
    {
        return;
    }

    node = find_card_node(card_name);
    if (node == 0)
    {
        printf("卡号不存在。\n");
        wait_enter();
        return;
    }

    while (1)
    {
        char choice_text[INPUT_BUFFER_SIZE];
        int choice = -1;

        printf("\n当前卡信息：\n");
        printf("卡号\t状态\t余额\t截止时间\t累计使用\t使用次数\t上次使用时间\n");
        print_card_detail_row(&node->data);
        printf("\n1. 修改密码\n");
        printf("2. 修改状态(0离线/1在线/2注销)\n");
        printf("3. 修改余额\n");
        printf("4. 删除当前卡（仅管理员）\n");
        printf("0. 返回\n");

        if (!read_line("请选择修改项：", choice_text, sizeof(choice_text)))
        {
            print_cancelled();
            return;
        }
        if (is_cancel_command(choice_text))
        {
            print_cancelled();
            return;
        }
        if (!try_parse_int(choice_text, &choice))
        {
            printf("输入无效。\n");
            continue;
        }

        if (choice == 0)
        {
            return;
        }

        if (choice == 1)
        {
            char new_pwd[INPUT_BUFFER_SIZE];
            char before_text[64];
            char after_text[64];

            if (!read_valid_password("请输入新密码(6位数字)：", new_pwd, sizeof(new_pwd)))
            {
                continue;
            }

            snprintf(before_text, sizeof(before_text), "%s", node->data.aPwd);
            snprintf(after_text, sizeof(after_text), "%s", new_pwd);
            strncpy(node->data.aPwd, new_pwd, sizeof(node->data.aPwd) - 1);
            node->data.aPwd[sizeof(node->data.aPwd) - 1] = '\0';
            card_service_save();
            auth_append_audit("修改密码", node->data.aName, before_text, after_text);
            printf("密码修改成功。\n");
            continue;
        }

        if (choice == 2)
        {
            char status_text[INPUT_BUFFER_SIZE];
            int new_status;
            char before_text[64];
            char after_text[64];

            if (!read_line("请输入新状态(0/1/2)：", status_text, sizeof(status_text)))
            {
                print_cancelled();
                return;
            }
            if (is_cancel_command(status_text))
            {
                print_cancelled();
                return;
            }
            if (!try_parse_int(status_text, &new_status) || new_status < 0 || new_status > 2)
            {
                printf("状态输入无效。\n");
                continue;
            }

            snprintf(before_text, sizeof(before_text), "%d", node->data.nStatus);
            snprintf(after_text, sizeof(after_text), "%d", new_status);
            node->data.nStatus = new_status;
            if (new_status == CARD_STATUS_DELETED)
            {
                node->data.nDel = 1;
                node->data.tEnd = time(0);
            }
            else
            {
                node->data.nDel = 0;
                node->data.tEnd = 0;
            }
            card_service_save();
            auth_append_audit("修改状态", node->data.aName, before_text, after_text);
            printf("状态修改成功。\n");
            continue;
        }

        if (choice == 3)
        {
            char amount_text[INPUT_BUFFER_SIZE];
            float new_balance;
            float old_balance;
            char before_text[64];
            char after_text[64];

            if (!read_line("请输入新余额(元)：", amount_text, sizeof(amount_text)))
            {
                print_cancelled();
                return;
            }
            if (is_cancel_command(amount_text))
            {
                print_cancelled();
                return;
            }
            if (!try_parse_float(amount_text, &new_balance))
            {
                printf("余额输入无效。\n");
                continue;
            }

            old_balance = node->data.fBalance;
            snprintf(before_text, sizeof(before_text), "%.2f", old_balance);
            snprintf(after_text, sizeof(after_text), "%.2f", new_balance);
            node->data.fBalance = new_balance;
            node->data.tLast = time(0);
            card_service_save();
            record_money_change(node->data.aName,
                                ADJUST_CARD,
                                new_balance - old_balance,
                                old_balance,
                                node->data.fBalance,
                                node->data.tLast);
            auth_append_audit("修改余额", node->data.aName, before_text, after_text);
            printf("余额修改成功。\n");
            continue;
        }

        if (choice == 4)
        {
            CardNode* prev = 0;
            char deleted_name[CARD_NAME_LEN + 1];
            char refund_time_text[32];
            float refund_amount = 0.0f;
            time_t delete_time;

            if (node->data.nStatus == CARD_STATUS_ONLINE)
            {
                printf("该卡正在上机，无法删除。\n");
                wait_enter();
                continue;
            }

            if (node->data.fBalance < 0.0f)
            {
                printf("当前余额为 %.2f 元（欠费），欠费状态下不允许删除，请先充值补缴。\n", node->data.fBalance);
                wait_enter();
                continue;
            }

            if (!confirm_delete_card_two_steps(node->data.aName))
            {
                continue;
            }

            strncpy(deleted_name, node->data.aName, sizeof(deleted_name) - 1);
            deleted_name[sizeof(deleted_name) - 1] = '\0';
            delete_time = time(0);

            if (node->data.fBalance > 0.0f)
            {
                float old_balance = node->data.fBalance;
                refund_amount = node->data.fBalance;
                node->data.fBalance = 0.0f;
                node->data.tLast = delete_time;
                record_money_change(node->data.aName,
                                    PHYSICAL_DELETE_CARD,
                                    refund_amount,
                                    old_balance,
                                    node->data.fBalance,
                                    node->data.tLast);
            }

            if (g_card_head == node)
            {
                g_card_head = node->next;
            }
            else
            {
                prev = g_card_head;
                while (prev != 0 && prev->next != node)
                {
                    prev = prev->next;
                }
                if (prev != 0)
                {
                    prev->next = node->next;
                }
            }

            free(node);
            card_service_save();

            if (refund_amount > 0.0f)
            {
                time_to_string(delete_time, refund_time_text, sizeof(refund_time_text));
                printf("-------------自动退费成功-------------\n");
                printf("卡号\t退费金额<元>\t退费时间\n");
                printf("%s\t%.2f\t\t%s\n", deleted_name, refund_amount, refund_time_text);
            }
            else
            {
                printf("当前余额为0.00元，本次无可退金额。\n");
            }

            auth_append_audit("删除账号", deleted_name, "存在", "已永久删除");
            printf("-------------删除成功-------------\n");
            printf("卡号 %s 已被永久删除。\n", deleted_name);
            wait_enter();
            return;
        }

        printf("输入无效，请输入 0~4。\n");
    }
}

/* 删除确认：仅需输入 YES/yes。 */
static int confirm_delete_card_two_steps(const char* card_name)
{
    char confirm_text[INPUT_BUFFER_SIZE];
    (void)card_name;

    if (!read_line("[删除确认] 请输入 YES 确认继续删除：", confirm_text, sizeof(confirm_text)))
    {
        print_cancelled();
        return 0;
    }
    if (is_cancel_command(confirm_text))
    {
        print_cancelled();
        return 0;
    }
    if (strcmp(confirm_text, "YES") != 0 && strcmp(confirm_text, "yes") != 0)
    {
        printf("确认失败，已取消删除。\n");
        wait_enter();
        return 0;
    }

    return 1;
}

/* 注销卡：仅修改状态为注销并保留账号数据，物理删除仅在管理员菜单内。 */
void card_service_delete_card(void)
{
    char card_name[INPUT_BUFFER_SIZE];
    CardNode* node;
    char confirm_text[INPUT_BUFFER_SIZE];
    char before_status_text[64];
    char after_status_text[64];
    char end_time_text[32];
    char refund_time_text[32];
    float refund_amount = 0.0f;
    time_t action_time;

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

    if (node->data.fBalance < 0.0f)
    {
        printf("当前余额为 %.2f 元（欠费），欠费状态下不允许注销，请先充值补缴。\n", node->data.fBalance);
        wait_enter();
        return;
    }

    if (!read_line("[注销确认] 请输入 YES 确认继续注销：", confirm_text, sizeof(confirm_text)))
    {
        return;
    }
    if (is_cancel_command(confirm_text))
    {
        print_cancelled();
        return;
    }
    if (strcmp(confirm_text, "YES") != 0 && strcmp(confirm_text, "yes") != 0)
    {
        printf("确认失败，已取消注销。\n");
        wait_enter();
        return;
    }

    /* 记录注销时刻；如果有正余额，先记一笔注销退费流水。 */
    action_time = time(0);

    /* 自动退所有费：注销时将正余额一次性退完并清零。 */
    if (node->data.fBalance > 0.0f)
    {
        float old_balance = node->data.fBalance;
        refund_amount = node->data.fBalance;
        node->data.fBalance = 0.0f;
        node->data.tLast = action_time;
        record_money_change(node->data.aName,
                            DELETE_CARD,
                            refund_amount,
                            old_balance,
                            node->data.fBalance,
                            node->data.tLast);
    }

    snprintf(before_status_text, sizeof(before_status_text), "%d", node->data.nStatus);
    node->data.nStatus = CARD_STATUS_DELETED;
    node->data.nDel = 1;
    node->data.tEnd = action_time;
    snprintf(after_status_text, sizeof(after_status_text), "%d", node->data.nStatus);
    card_service_save();
    auth_append_audit("注销账号", node->data.aName, before_status_text, after_status_text);

    if (refund_amount > 0.0f)
    {
        time_to_string(action_time, refund_time_text, sizeof(refund_time_text));
        printf("-------------自动退费成功-------------\n");
        printf("卡号\t退费金额<元>\t退费时间\n");
        printf("%s\t%.2f\t\t%s\n", card_name, refund_amount, refund_time_text);
    }
    else
    {
        printf("当前余额为0.00元，本次无可退金额。\n");
    }

    time_to_string(node->data.tEnd, end_time_text, sizeof(end_time_text));
    printf("-------------注销成功-------------\n");
    printf("卡号\t状态\t截止时间\n");
    printf("%s\t%s\t%s\n", card_name, status_to_text(node->data.nStatus), end_time_text);
    wait_enter();
}
