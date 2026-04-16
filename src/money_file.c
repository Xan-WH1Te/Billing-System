#include <money_file.h>
#include <tool.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MONEY_LINE_MAX_LEN 512

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

static int money_record_is_valid(const Money* record)
{
	if (record == 0)
	{
		return 0;
	}

	if (strlen(record->aCardName) != CARD_NAME_LEN || !is_all_digits(record->aCardName))
	{
		return 0;
	}
	if (record->nStatus < REGISTER_CARD || record->nStatus > PHYSICAL_DELETE_CARD)
	{
		return 0;
	}
	if (record->nDel != 0 && record->nDel != 1)
	{
		return 0;
	}
	if (!isfinite(record->fMoney) || !isfinite(record->fBeforeBalance) || !isfinite(record->fAfterBalance))
	{
		return 0;
	}

	return 1;
}

bool money_file_append(const Money* record, const char* file_path)
{
	FILE* file;
	int write_result;

	if (record == 0 || file_path == 0)
	{
		return false;
	}
	if (!money_record_is_valid(record))
	{
		return false;
	}

	file = fopen(file_path, "a");
	if (file == 0)
	{
		return false;
	}

	write_result = fprintf(file,
						"%s##%lld##%d##%.2f##%.2f##%.2f##%d\n",
						record->aCardName,
						(long long)record->tTime,
						record->nStatus,
						record->fMoney,
						record->fBeforeBalance,
						record->fAfterBalance,
						record->nDel);

	if (write_result < 0)
	{
		fclose(file);
		return false;
	}

	if (fclose(file) != 0)
	{
		return false;
	}

	return true;
}

bool money_file_load_all(const char* file_path, Money** out_records, size_t* out_count)
{
	FILE* file;
	char line[MONEY_LINE_MAX_LEN];
	Money* records = 0;
	size_t count = 0;
	size_t capacity = 0;

	if (file_path == 0 || out_records == 0 || out_count == 0)
	{
		return false;
	}

	*out_records = 0;
	*out_count = 0;

	file = fopen(file_path, "r");
	if (file == 0)
	{
		return true;
	}

	while (fgets(line, sizeof(line), file) != 0)
	{
		Money record;
		long long t_time;
		size_t line_length;

		line_length = strlen(line);
		if (line_length > 0 && line[line_length - 1] != '\n')
		{
			discard_until_newline(file);
			continue;
		}

		trim_newline(line);
		if (line[0] == '\0')
		{
			continue;
		}

		memset(&record, 0, sizeof(record));
		if (sscanf(line,
				   "%7[^#]##%lld##%d##%f##%f##%f##%d",
				   record.aCardName,
				   &t_time,
				   &record.nStatus,
				   &record.fMoney,
				   &record.fBeforeBalance,
				   &record.fAfterBalance,
				   &record.nDel) != 7)
		{
			continue;
		}

		record.tTime = (time_t)t_time;
		if (!money_record_is_valid(&record))
		{
			continue;
		}

		if (count == capacity)
		{
			size_t new_capacity;
			Money* resized;

			if (capacity == 0)
			{
				new_capacity = 32;
			}
			else
			{
				if (capacity > ((size_t)-1) / 2)
				{
					free(records);
					fclose(file);
					return false;
				}
				new_capacity = capacity * 2;
			}

			resized = (Money*)realloc(records, new_capacity * sizeof(Money));
			if (resized == 0)
			{
				free(records);
				fclose(file);
				return false;
			}

			records = resized;
			capacity = new_capacity;
		}

		records[count] = record;
		++count;
	}

	fclose(file);

	*out_records = records;
	*out_count = count;
	return true;
}

void money_file_free_all(Money* records)
{
	free(records);
}
