#include "Core/Tetris.h"
#include "Rendering/Console.h"
#include "Rendering/Splash.h"
#include "Menu/GameMenu.h"
#include "Input/Settings.h"
#include "Agent/AgentGame.h"
#include "Agent/McpServer.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // Режимы запуска:
    //   (без флагов)      — обычная игра (заставка, меню, ввод с клавиатуры);
    //   --agent           — агентский режим: сразу в игру, без заставки/меню,
    //                       ходы — из агентского канала (TCP 127.0.0.1:7777-7786);
    //   --mcp-server      — MCP-сервер (stdio JSON-RPC), отдельный процесс:
    //                       подключается к игре, запущенной с --agent.
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "--agent")
        {
            Tetris::ProgramSettings();
            // Инициализация базового буфера (как в обычной игре): без cls
            // курсор не в (0,0) и в буфере остаётся текст командной строки —
            // двойная буферизация не заводилась (моргание со смещением по
            // вертикали в Fixed).
            std::wcout << std::endl;
            system("cls");
            Tetris::PlayAgentGame();
            return 0;
        }

        if (arg == "--mcp-server")
        {
            return Tetris::RunMcpServer();
        }
    }

    Tetris::ProgramSettings();

    // Настройки клавиш из tetris_settings.ini
    Tetris::LoadBindings(Tetris::g_bindings);

    // Инициализация базового буфера
    std::wcout << std::endl;
    system("cls");

    // Заставка: логотип, висит до нажатия любой клавиши
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
