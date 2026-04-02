#include <menu.h>
#include <service.h>

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* 程序入口：初始化服务、进入菜单循环、退出时清理资源。 */
int main(void)
{
#ifdef _WIN32
    /* Windows 控制台切换为 UTF-8，避免中文乱码。 */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    service_init();
    menu_loop();
    service_cleanup();

    return 0;
}
