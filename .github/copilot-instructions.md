# Инструкции для Copilot

Полное руководство для агентов — в [AGENTS.md](../AGENTS.md). Кратко:

- Консольный Тетрис на C++ (MSVC v145, Win32/x64, C++23), без внешних зависимостей.
- Код: `Tetris/Main.cpp`, `Tetris/App.h`, `Tetris/Tetris.cpp`, `Tetris/Figure.h`/`Figure.cpp`, `Tetris/GameData.h`, `Tetris/Keys.h`, `Tetris/Settings.h`.
- После изменений — сборка без ошибок и предупреждений.
- Ввод: игровой цикл — `GetAsyncKeyState`, меню — `_getch`; консоль 80×25, unicode-режим.
- Предупреждения анализа гасить `(void)вызов()`, не `#pragma warning(disable)`.
- `using namespace` запрещён — только точечные `using`-декларации.
- `README.md` всегда держать актуальным (обновлять вместе с изменениями кода).
