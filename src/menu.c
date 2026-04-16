#include <menu.h>
#include <service.h>
#include <tool.h>

#include <stdio.h>

/* 读取并校验菜单编号输入，仅接受 0~9 的纯数字输入。 */
static int read_menu_choice(int* out_choice)
{
    char line[64];
    int parsed_choice;

    if (!read_line_safely(NULL, line, sizeof(line)))
    {
        return 0;
    }

    if (!parse_int32_strict(line, &parsed_choice))
    {
        return 0;
    }

    if (parsed_choice < 0 || parsed_choice > 9)
    {
        return 0;
    }

    *out_choice = parsed_choice;
    return 1;
}

/* 主菜单循环：展示菜单、读取选择并分发到 service 层。 */
void menu_loop(void)
{
    printf("|--------------------------|\n");
    printf("|皇家大理工电子阅览室欢迎您|\n");
    printf("|--------------------------|\n");
    while (1)
    {
        int choice = -1;
        printf("   ---------菜单---------\n");
        printf("1. 添加卡\n");
        printf("2. 查询卡\n");
        printf("3. 上机\n");
        printf("4. 下机\n");
        printf("5. 充值\n");
        printf("6. 退费\n");
        printf("7. 查询统计\n");
        printf("8. 注销卡\n");
        printf("9. 管理员\n");
        printf("0. 退出\n");
        if (service_is_admin_logged_in())
        {
            printf("@Admin logon\n");
        }
        printf("请选择菜单项编号：");

        if (!read_menu_choice(&choice))
        {
            /* 输入解析失败时提示并等待用户继续。 */
            printf("\n输入无效，请输入数字。\n");
            wait_enter();
            continue;
        }

        if (choice == 0)
        {
            printf("\n已退出系统。\n");
            return;
        }

        switch (choice)
        {
        /* 菜单编号与业务动作一一对应。 */
        case 1: service_add_card(); break;
        case 2: service_query_card(); break;
        case 3: service_start_session(); break;
        case 4: service_end_session(); break;
        case 5: service_recharge(); break;
        case 6: service_refund(); break;
        case 7: service_query_stats(); break;
        case 8: service_delete_card(); break;
        case 9: service_admin_portal(); break;
        default: break;
        }
    }
}
