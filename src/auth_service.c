#include <auth_service.h>
#include <global.h>
#include <tool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ADMIN_NAME_MAX_LEN 31
#define ADMIN_PWD_MAX_LEN 63

static char g_admin_name[ADMIN_NAME_MAX_LEN + 1] = "Admin";
static char g_admin_pwd[ADMIN_PWD_MAX_LEN + 1] = "123456";
static int g_admin_logged_in = 0;
static int g_admin_failed_count = 0;
static int g_admin_locked = 0;

static int parse_admin_line(const char* line, char* out_name, char* out_pwd)
{
    return sscanf(line, "%31[^#]##%63[^\r\n]", out_name, out_pwd) == 2;
}

static void save_admin_file(void)
{
    FILE* file = fopen(ADMIN_FILE_PATH, "w");
    int result;

    if (file == 0)
    {
        printf("警告：管理员配置保存失败。\n");
        return;
    }

    result = fprintf(file, "%s##%s\n", g_admin_name, g_admin_pwd);
    if (result < 0)
    {
        printf("警告：管理员配置写入失败。\n");
    }
    fclose(file);
}

void auth_init(void)
{
    FILE* file = fopen(ADMIN_FILE_PATH, "r");
    char line[256];

    if (file == 0)
    {
        save_admin_file();
        return;
    }

    if (fgets(line, sizeof(line), file) != 0)
    {
        char name[ADMIN_NAME_MAX_LEN + 1];
        char pwd[ADMIN_PWD_MAX_LEN + 1];

        trim_newline(line);
        if (parse_admin_line(line, name, pwd))
        {
            strncpy(g_admin_name, name, sizeof(g_admin_name) - 1);
            g_admin_name[sizeof(g_admin_name) - 1] = '\0';
            strncpy(g_admin_pwd, pwd, sizeof(g_admin_pwd) - 1);
            g_admin_pwd[sizeof(g_admin_pwd) - 1] = '\0';
        }
    }

    fclose(file);
}

void auth_cleanup(void)
{
    g_admin_logged_in = 0;
    g_admin_failed_count = 0;
    g_admin_locked = 0;
}

bool auth_is_admin(void)
{
    return g_admin_logged_in == 1;
}

const char* auth_current_admin(void)
{
    if (!auth_is_admin())
    {
        return "-";
    }
    return g_admin_name;
}

void auth_login_admin(void)
{
    char input_name[INPUT_BUFFER_SIZE];
    char input_pwd[INPUT_BUFFER_SIZE];

    if (auth_is_admin())
    {
        printf("当前已是管理员登录状态：%s\n", g_admin_name);
        wait_enter();
        return;
    }

    if (g_admin_locked)
    {
        printf("管理员登录已锁定（失败次数达到5次），请重启程序后再试。\n");
        wait_enter();
        return;
    }

    printf("--------管理员登录--------\n");
    if (!read_line_safely("请输入管理员账号（q/Q 取消）：", input_name, sizeof(input_name)))
    {
        printf("已取消。\n");
        return;
    }
    if (is_cancel_command(input_name))
    {
        printf("已取消。\n");
        return;
    }

    if (!read_line_safely("请输入管理员密码（q/Q 取消）：", input_pwd, sizeof(input_pwd)))
    {
        printf("已取消。\n");
        return;
    }
    if (is_cancel_command(input_pwd))
    {
        printf("已取消。\n");
        return;
    }

    if (strcmp(input_name, g_admin_name) != 0 || strcmp(input_pwd, g_admin_pwd) != 0)
    {
        g_admin_failed_count += 1;
        if (g_admin_failed_count >= 5)
        {
            g_admin_locked = 1;
            printf("管理员账号或密码错误，失败次数达到5次，登录已锁定。\n");
            wait_enter();
            return;
        }

        printf("管理员账号或密码错误。\n");
        printf("当前失败次数：%d/5\n", g_admin_failed_count);
        wait_enter();
        return;
    }

    g_admin_failed_count = 0;
    g_admin_locked = 0;
    g_admin_logged_in = 1;
    printf("管理员登录成功。\n");
    wait_enter();
}

void auth_logout_admin(void)
{
    if (!auth_is_admin())
    {
        printf("当前未登录管理员。\n");
        wait_enter();
        return;
    }

    g_admin_logged_in = 0;
    printf("管理员已退出。\n");
    wait_enter();
}

bool auth_append_audit(const char* action,
                       const char* target_card,
                       const char* before_value,
                       const char* after_value)
{
    FILE* file;
    time_t now;
    char time_text[32];

    if (!auth_is_admin() || action == 0 || target_card == 0 || before_value == 0 || after_value == 0)
    {
        return false;
    }

    file = fopen(ADMIN_AUDIT_FILE_PATH, "a");
    if (file == 0)
    {
        return false;
    }

    now = time(0);
    time_to_string(now, time_text, sizeof(time_text));
    fprintf(file,
            "%s##%s##%s##%s##%s##%s\n",
            time_text,
            g_admin_name,
            target_card,
            action,
            before_value,
            after_value);

    fclose(file);
    return true;
}
