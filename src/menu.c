#include <menu.h>
#include <service.h>
#include <tool.h>

#include <stdio.h>

static int read_menu_choice(int* out_choice)
{
    char line[64];
    char extra;

    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        return 0;
    }

    if (sscanf(line, "%d %c", out_choice, &extra) != 1)
    {
        return 0;
    }

    return *out_choice >= 0 && *out_choice <= 8;
}

void menu_loop(void)
{
    while (1)
    {
        int choice = -1;

        printf("皇家大理工电子阅览室欢迎您\n");
        printf("---------菜单---------\n");
        printf("1. 添加卡\n");
        printf("2. 查询卡\n");
        printf("3. !上机\n");
        printf("4. !下机\n");
        printf("5. !充值\n");
        printf("6. !退费\n");
        printf("7. !查询统计\n");
        printf("8. !注销卡\n");
        printf("0. 退出\n");
        printf("请选择菜单项编号, 键入q/Q以退回到主菜单：");

        if (!read_menu_choice(&choice))
        {
            printf("\n输入无效，请输入数字。 按回车键继续...");
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
        case 1: service_add_card(); break;
        case 2: service_query_card(); break;
        case 3: service_start_session(); break;
        case 4: service_end_session(); break;
        case 5: service_recharge(); break;
        case 6: service_refund(); break;
        case 7: service_query_stats(); break;
        case 8: service_delete_card(); break;
        default: break;
        }
    }
}
