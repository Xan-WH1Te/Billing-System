#include <service.h>
#include <auth_service.h>
#include <card_service.h>
#include <tool.h>

#include <stdio.h>

/* 初始化各业务子服务。 */
void service_init(void)
{
    auth_init();
    card_service_load();
}

/* 统一释放各业务子服务资源。 */
void service_cleanup(void)
{
    card_service_free();
    auth_cleanup();
}

/* 以下函数作为菜单层到卡业务层的转发门面。 */
void service_add_card(void)
{
    card_service_add_card();
}

void service_query_card(void)
{
    card_service_query_card();
}

void service_start_session(void)
{
    card_service_start_session();
}

void service_end_session(void)
{
    card_service_end_session();
}

void service_recharge(void)
{
    card_service_recharge();
}

void service_refund(void)
{
    card_service_refund();
}

void service_query_stats(void)
{
    card_service_query_stats();
}

void service_delete_card(void)
{
    card_service_delete_card();
}

int service_is_admin_logged_in(void)
{
    return auth_is_admin() ? 1 : 0;
}

void service_admin_login(void)
{
    auth_login_admin();
}

void service_admin_logout(void)
{
    auth_logout_admin();
}

void service_admin_edit_card(void)
{
    if (!auth_is_admin())
    {
        printf("请先进行管理员登录。\n");
        wait_enter();
        return;
    }

    card_service_admin_edit_card();
}

void service_admin_portal(void)
{
    while (1)
    {
        int choice = -1;
        char line[64];

        printf("--------管理员入口--------\n");
        if (auth_is_admin())
        {
            printf("当前状态：已登录管理员\n");
            printf("1. 直接修改卡信息\n");
            printf("2. 计费标准设置\n");
            printf("3. 退出管理员登录\n");
        }
        else
        {
            printf("当前状态：未登录管理员\n");
            printf("1. 管理员登录\n");
        }
        printf("0. 返回主菜单\n");
        printf("请选择菜单项：");

        if (!read_line_safely(NULL, line, sizeof(line)))
        {
            return;
        }

        if (!parse_int32_strict(line, &choice))
        {
            printf("输入无效，请输入数字。\n");
            wait_enter();
            continue;
        }

        if (choice == 0)
        {
            return;
        }

        if (!auth_is_admin())
        {
            if (choice == 1)
            {
                auth_login_admin();
                continue;
            }

            printf("输入无效，请输入 0~1。\n");
            wait_enter();
            continue;
        }

        if (choice == 1)
        {
            card_service_admin_edit_card();
            continue;
        }
        if (choice == 2)
        {
            card_service_admin_manage_pricing();
            continue;
        }
        if (choice == 3)
        {
            auth_logout_admin();
            continue;
        }

        printf("输入无效，请输入 0~3。\n");
        wait_enter();
    }
}
