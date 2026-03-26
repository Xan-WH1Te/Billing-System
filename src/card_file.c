#include <card_file.h>
#include <tool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CARD_LINE_MAX_LEN 512

static const char* status_to_text(CardStatus status)
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

static bool text_to_status(const char* text, CardStatus* out_status)
{
    if (strcmp(text, "Active") == 0)
    {
        *out_status = Active;
        return true;
    }
    if (strcmp(text, "LoggedOut") == 0)
    {
        *out_status = LoggedOut;
        return true;
    }
    if (strcmp(text, "OutofDate") == 0)
    {
        *out_status = OutofDate;
        return true;
    }
    return false;
}

static bool append_parsed_card(CardNode** head, CardNode** tail, const Card* card)
{
    CardNode* node;

    if (head == NULL || tail == NULL || card == NULL)
    {
        return false;
    }

    node = (CardNode*)malloc(sizeof(CardNode));
    if (node == NULL)
    {
        return false;
    }

    node->card = *card;
    node->next = NULL;

    if (*head == NULL)
    {
        *head = node;
        *tail = node;
    }
    else
    {
        (*tail)->next = node;
        *tail = node;
    }

    return true;
}

static void write_card_line(FILE* file, const Card* card)
{
    fprintf(file,
            "%d##%s##%s##%lld##%s##%s##%s##%s\n",
            card->id,
            card->ownerName,
            card->pin,
            card->balanceCent,
            status_to_text(card->status),
            card->openTime,
            card->expireTime,
            card->lastUseTime);
}

bool save_all_cards(const CardNode* head, const char* file_path)
{
    FILE* file;
    const CardNode* current;

    if (file_path == NULL)
    {
        return false;
    }

    file = fopen(file_path, "w");
    if (file == NULL)
    {
        return false;
    }

    current = head;
    while (current != NULL)
    {
        write_card_line(file, &current->card);
        current = current->next;
    }

    fclose(file);
    return true;
}

bool parse_card_line(const char* line, Card* out_card)
{
    char buffer[CARD_LINE_MAX_LEN];
    char* fields[8];
    int count = 0;
    char* start;
    char* pos;
    time_t parsed_time;

    if (line == NULL || out_card == NULL)
    {
        return false;
    }

    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    start = buffer;
    while (count < 8)
    {
        pos = strstr(start, "##");
        if (pos == NULL)
        {
            fields[count++] = start;
            break;
        }

        *pos = '\0';
        fields[count++] = start;
        start = pos + 2;
    }

    if (count != 8)
    {
        return false;
    }

    out_card->id = atoi(fields[0]);
    strncpy(out_card->ownerName, fields[1], OWNER_NAME_MAX_LEN);
    out_card->ownerName[OWNER_NAME_MAX_LEN] = '\0';

    strncpy(out_card->pin, fields[2], PIN_MAX_LEN);
    out_card->pin[PIN_MAX_LEN] = '\0';

    out_card->balanceCent = atoll(fields[3]);

    if (!text_to_status(fields[4], &out_card->status))
    {
        return false;
    }

    if (!string_to_time(fields[5], &parsed_time))
    {
        return false;
    }

    if (strcmp(fields[6], "永久") != 0 && !string_to_time(fields[6], &parsed_time))
    {
        return false;
    }

    if (strcmp(fields[7], "-") != 0 && !string_to_time(fields[7], &parsed_time))
    {
        return false;
    }

    strncpy(out_card->openTime, fields[5], TIME_TEXT_MAX_LEN);
    out_card->openTime[TIME_TEXT_MAX_LEN] = '\0';

    strncpy(out_card->expireTime, fields[6], TIME_TEXT_MAX_LEN);
    out_card->expireTime[TIME_TEXT_MAX_LEN] = '\0';

    strncpy(out_card->lastUseTime, fields[7], TIME_TEXT_MAX_LEN);
    out_card->lastUseTime[TIME_TEXT_MAX_LEN] = '\0';

    out_card->inSession = 0;
    return true;
}

CardNode* read_cards(const char* file_path)
{
    FILE* file;
    char line[CARD_LINE_MAX_LEN];
    CardNode* head = NULL;
    CardNode* tail = NULL;

    if (file_path == NULL)
    {
        return NULL;
    }

    file = fopen(file_path, "r");
    if (file == NULL)
    {
        return NULL;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        size_t len = strlen(line);
        Card parsed_card;
        CardNode* node;

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[len - 1] = '\0';
            --len;
        }

        if (len == 0)
        {
            continue;
        }

        if (!parse_card_line(line, &parsed_card))
        {
            continue;
        }

        if (!append_parsed_card(&head, &tail, &parsed_card))
        {
            fclose(file);
            return head;
        }
    }

    fclose(file);
    return head;
}
