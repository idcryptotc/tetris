#include "../Core/GameData.h"
#include "../Input/Keys.h"
#include "../Input/Settings.h"
#include "GameMenu.h"

#include <cstdlib>
#include <iostream>

#include <conio.h>
#include <windows.h>

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
        // Игровой цикл заметил нажатие клавиши выхода, но она ещё может быть
        // физически нажата. Дожидаемся отпускания, иначе первое чтение в меню
        // сразу вернёт её (Esc) и меню мгновенно закроется.
        while (IsKeyPressed(KeyFor(BindingAction::EXIT)))
        {
            Sleep(10);
        }

        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Продолжить" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;

            int key = _getch();

            if (key == static_cast<int>(KeyCodes::ESCAPE))
            {
                // Esc — то же, что пункт 2. Дожидаемся отпускания клавиши,
                // чтобы после возврата в игру IsKeyPressed(EXIT) не сработал сразу.
                while (IsKeyPressed(KeyCodes::ESCAPE))
                {
                    Sleep(10);
                }

                return MenuResult::Stay;
            }

            menuItem = key - '0';

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
