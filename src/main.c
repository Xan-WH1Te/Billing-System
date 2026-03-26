#include <menu.h>
#include <service.h>

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    service_init();
    menu_loop();
    service_cleanup();

    return 0;
}
