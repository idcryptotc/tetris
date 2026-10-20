# CLAUDE.md

Полное руководство для агентов — в [AGENTS.md](AGENTS.md). Кратко:

- Консольный Тетрис на C++ (MSVC v145, x64/Win32, C++23, CMake), без внешних зависимостей.
- Код: `Tetris/Tetris.cpp`, `Tetris/Figure.h`, `Tetris/Figure.cpp`, `Tetris/Game.h`, `Tetris/Keys.h`, `Tetris/Settings.h`.
- После изменений — сборка без ошибок и предупреждений.
- Ввод только через `GetAsyncKeyState`; консоль 80×25, unicode-режим.
- Предупреждения анализа гасить `(void)вызов()`, не `#pragma warning(disable)`.
- `using namespace` запрещён — только точечные `using`-декларации.
- `README.md` всегда держать актуальным (обновлять вместе с изменениями кода).
