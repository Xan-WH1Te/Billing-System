#include <card_system.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    bool readMenuChoice(int& choice)
    {
        std::string line;
        if (!std::getline(std::cin, line))
        {
            return false;
        }

        std::stringstream ss(line);
        int value = -1;
        char extra = '\0';

        if (!(ss >> value) || (ss >> extra) || value < 0 || value > 8)
        {
            return false;
        }

        choice = value;
        return true;
    }
}


int main() 
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    CardService::loadCardsFromFile();

    while (true)
    {

        std::cout
            << "皇家大理工电子阅览室欢迎您\n"
            << "---------菜单---------\n"
            << "1. 添加卡\n"
            << "2. 查询卡\n"
            << "3. !上机\n"
            << "4. !下机\n"
            << "5. !充值\n"
            << "6. !退费\n"
            << "7. !查询统计\n"
            << "8. !注销卡\n"
            << "0. 退出\n"
            << "请选择菜单项编号, 键入q/Q以退回到主菜单：";
        
        int choice = -2;
        if (!readMenuChoice(choice))
        {
            std::cin.clear();
            std::cout << "\n输入无效，请输入数字。 按回车键继续...";
            std::cin.get();
            continue;
        }
    
        if (choice == 0)
        {   
            std::cout << "\n已退出系统。\n";
            break;
        }
        switch (choice)
        {
            case 1: CardService::addCard(); break;
            case 2: CardService::queryCard(); break;
            // case 3: CardService::startSession(); break;
            // case 4: CardService::endSession(); break;
            // case 5: CardService::recharge(); break;
            // case 6: CardService::refund(); break;
            // case 7: CardService::queryStats(); break;
            // case 8: CardService::deleteCard(); break;
        }   

    }

    return 0;
}