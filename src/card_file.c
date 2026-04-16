#include <card_file.h>
#include <tool.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARD_LINE_MAX_LEN 512

static int parse_money_storage_token(const char* token, MoneyCent* out_cent)
{
    if (token == 0 || out_cent == 0)
    {
        return 0;
    }

    /* 兼容旧格式（元小数）与新格式（分整数）。 */
    if (strchr(token, '.') != 0)
    {
        return parse_yuan_to_cent_strict(token, out_cent) ? 1 : 0;
    }

    return parse_int64_strict(token, out_cent) ? 1 : 0;
}

static void free_all_nodes(CardNode* head)
{
    while (head != 0)
    {
        CardNode* next = head->next;
        free(head);
        head = next;
    }
}

static void discard_until_newline(FILE* file)
{
    int ch;

    while ((ch = fgetc(file)) != '\n' && ch != EOF)
    {
    }
}

static int is_all_digits(const char* text)
{
    size_t i;
    size_t length;

    if (text == 0)
    {
        return 0;
    }

    length = strlen(text);
    if (length == 0)
    {
        return 0;
    }

    for (i = 0; i < length; ++i)
    {
        if (!isdigit((unsigned char)text[i]))
        {
            return 0;
        }
    }

    return 1;
}

static int card_record_is_valid(const Card* card)
{
    if (card == 0)
    {
        return 0;
    }

    if (strlen(card->aName) != CARD_NAME_LEN || !is_all_digits(card->aName))
    {
        return 0;
    }
    if (strlen(card->aPwd) != CARD_PWD_LEN || !is_all_digits(card->aPwd))
    {
        return 0;
    }
    if (card->nStatus < CARD_STATUS_OFFLINE || card->nStatus > CARD_STATUS_DELETED)
    {
        return 0;
    }
    if (card->nDel != 0 && card->nDel != 1)
    {
        return 0;
    }
    if (card->nUseCount < 0)
    {
        return 0;
    }
    if (!isfinite(card->fTotalUse))
    {
        return 0;
    }

    return 1;
}

/* 将一张卡追加到链表尾部，成功返回1，失败返回0。 */
static int append_node(CardNode** head, CardNode** tail, const Card* card)
{
    CardNode* node = (CardNode*)malloc(sizeof(CardNode));

    if (node == 0)
    {
        return 0;
    }

    node->data = *card;
    node->next = 0;

    if (*head == 0)
    {
        *head = node;
        *tail = node;
    }
    else
    {
        (*tail)->next = node;
        *tail = node;
    }

    return 1;
}

/* 将内存中的卡链表按固定文本格式保存到磁盘。 */
bool card_file_save_all(const CardNode* head, const char* file_path)
{
    FILE* file;
    const CardNode* current;
    int write_result;

    if (file_path == 0)
    {
        return false;
    }

    file = fopen(file_path, "w");
    if (file == 0)
    {
        return false;
    }

    current = head;
    while (current != 0)
    {
        /* 以 "字段##字段" 的行格式写出，便于后续按行反序列化。 */
        /* 顺序固定：卡号, 密码, 状态, 开卡, 截止, 累计使用, 上次使用, 使用次数, 余额, 删除标记。 */
        write_result = fprintf(file,
                               "%s##%s##%d##%lld##%lld##%.2f##%lld##%d##%lld##%d\n",
                               current->data.aName,
                               current->data.aPwd,
                               current->data.nStatus,
                               (long long)current->data.tStart,
                               (long long)current->data.tEnd,
                               current->data.fTotalUse,
                               (long long)current->data.tLast,
                               current->data.nUseCount,
                               (long long)current->data.nBalanceCent,
                               current->data.nDel);
        if (write_result < 0)
        {
            fclose(file);
            return false;
        }
        current = current->next;
    }

    if (fclose(file) != 0)
    {
        return false;
    }

    return true;
}

/* 从磁盘读取卡数据并重建链表，忽略格式不合法的行。 */
CardNode* card_file_load_all(const char* file_path)
{
    FILE* file;
    char line[CARD_LINE_MAX_LEN];
    CardNode* head = 0;
    CardNode* tail = 0;

    if (file_path == 0)
    {
        return 0;
    }

    file = fopen(file_path, "r");
    if (file == 0)
    {
        return 0;
    }

    while (fgets(line, sizeof(line), file) != 0)
    {
        Card card;
        long long t_start;
        long long t_end;
        long long t_last;
        char balance_text[64];
        size_t line_length;

        line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] != '\n')
        {
            /* 单条记录超长时丢弃余下内容，避免污染下一行。 */
            discard_until_newline(file);
            continue;
        }

        /* 空行直接跳过，避免无意义解析。 */
        trim_newline(line);
        if (line[0] == '\0')
        {
            continue;
        }

        memset(&card, 0, sizeof(card));
        /* 解析失败说明该行损坏，跳过并继续读取后续数据。 */
        if (sscanf(line,
                   "%7[^#]##%6[^#]##%d##%lld##%lld##%f##%lld##%d##%63[^#]##%d",
                   card.aName,
                   card.aPwd,
                   &card.nStatus,
                   &t_start,
                   &t_end,
                   &card.fTotalUse,
                   &t_last,
                   &card.nUseCount,
                   balance_text,
                   &card.nDel) != 10)
        {
            continue;
        }

        if (!parse_money_storage_token(balance_text, &card.nBalanceCent))
        {
            continue;
        }

        card.tStart = (time_t)t_start;
        card.tEnd = (time_t)t_end;
        card.tLast = (time_t)t_last;

        if (!card_record_is_valid(&card))
        {
            continue;
        }

        /* 追加失败通常是内存不足，释放已加载数据并失败返回。 */
        if (!append_node(&head, &tail, &card))
        {
            free_all_nodes(head);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return head;
}

