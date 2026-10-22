#include "../Core/GameData.h"
#include "GameMenu.h"

#include <cstdlib>
#include <iostream>

#include <conio.h>

namespace Tetris
{
    int StartMenu()
    {
        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Настройка клавиш" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;
            menuItem = _getch() - '0';

            if (menuItem < 1 || menuItem > 3)
            {
                system("cls");
                continue;
            }

            system("cls");
            return menuItem;
        }
    }

    MenuResult ReturnToMenu()
    {
        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Продолжить" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;
            menuItem = _getch() - '0';

            if (menuItem < 1 || menuItem > 3)
            {
                system("cls");
            }
            else
            {
                break;
            }
        }

        return static_cast<MenuResult>(menuItem);
    }

    void ClearKeyboardBuffer()
    {
        while (_kbhit())
        {
            (void)_getch(); // чистим буфер, символ не нужен
        }
    }
}
