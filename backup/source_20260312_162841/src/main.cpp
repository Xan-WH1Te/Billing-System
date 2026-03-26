#include <card_system.h>

#include <cstdlib>
#include <iostream>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

int main() 
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF7);
    SetConsoleCP(CP_UTF7);
#endif
    while (true)
    {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        std::cout
            << "皇家大理工电子阅览室欢迎您\n"
            << "---------菜单---------\n"
            << "0. 添加卡\n"
            << "1. 查询卡\n"
            << "2. 上机\n"
            << "3. 下机\n"
            << "4. 充值\n"
            << "5. 退费\n"
            << "6. 查询统计\n"
            << "7. 注销卡\n"
            << "8. 退出\n"
            << "请选择菜单项编号（0~8）：";
        
        int choice = -2;
        if (!(std::cin >> choice) || choice > 8 || choice < 0)
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\n输入无效，请输入数字。 按回车键继续...";
            std::cin.get();
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
        if (choice == 8)
        {   
            std::cout << "\n已退出系统。\n";
            break;
        }
        switch (choice)
        {
            case 0: CardService::addCard(); break;
            case 1: CardService::queryCard(); break;
            case 2: CardService::startSession(); break;
            case 3: CardService::endSession(); break;
            case 4: CardService::recharge(); break;
            case 5: CardService::refund(); break;
            case 6: CardService::queryStats(); break;
            case 7: CardService::deleteCard(); break;
        }   

    }

    return -1;
}