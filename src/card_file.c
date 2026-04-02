#include <card_file.h>
#include <tool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARD_LINE_MAX_LEN 512

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
        fprintf(file,
                "%s##%s##%d##%lld##%lld##%.2f##%lld##%d##%.2f##%d\n",
                current->data.aName,
                current->data.aPwd,
                current->data.nStatus,
                (long long)current->data.tStart,
                (long long)current->data.tEnd,
                current->data.fTotalUse,
                (long long)current->data.tLast,
                current->data.nUseCount,
                current->data.fBalance,
                current->data.nDel);
        current = current->next;
    }

    fclose(file);
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

        /* 空行直接跳过，避免无意义解析。 */
        trim_newline(line);
        if (line[0] == '\0')
        {
            continue;
        }

        memset(&card, 0, sizeof(card));
        /* 解析失败说明该行损坏，跳过并继续读取后续数据。 */
        if (sscanf(line,
                   "%7[^#]##%6[^#]##%d##%lld##%lld##%f##%lld##%d##%f##%d",
                   card.aName,
                   card.aPwd,
                   &card.nStatus,
                   &t_start,
                   &t_end,
                   &card.fTotalUse,
                   &t_last,
                   &card.nUseCount,
                   &card.fBalance,
                   &card.nDel) != 10)
        {
            continue;
        }

        card.tStart = (time_t)t_start;
        card.tEnd = (time_t)t_end;
        card.tLast = (time_t)t_last;

        /* 追加失败通常是内存不足，保留已加载数据并结束读取。 */
        if (!append_node(&head, &tail, &card))
        {
            break;
        }
    }

    fclose(file);
    return head;
}
