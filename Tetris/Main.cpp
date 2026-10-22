#include "Core/Tetris.h"
#include "Rendering/Console.h"
#include "Rendering/Splash.h"
#include "Menu/GameMenu.h"
#include "Input/Settings.h"

#include <cstdlib>

int main()
{
    Tetris::ProgramSettings();

    // Настройки клавиш из tetris_settings.ini
    Tetris::LoadBindings(Tetris::g_bindings);

    // Заставка: логотип, висит до нажатия любой клавиши
    std::wcout << std::endl;
    Tetris::ShowSplash();

    while (true)
    {
        system("cls");
        int menuItem = Tetris::StartMenu();

        if (menuItem == 3)
        {
            return 0; // Выход
        }

        if (menuItem == 2)
        {
            Tetris::ConfigureBindings(Tetris::g_bindings);
            continue; // Вернуться в меню, а не сразу в игру
        }

        if (!Tetris::PlayGame())
        {
            break; // Выбранный в игре «Выход»
        }

        // PlayGame() вернул true — конец партии, предлагаем сыграть снова
    }

    return 0;
}
