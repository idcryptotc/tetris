# CLAUDE.md

Полное руководство для агентов — в [AGENTS.md](AGENTS.md). Кратко:

- Консольный Тетрис на C++ (MSVC v145, Win32, C++23), без внешних зависимостей.
- Код: `Tetris/Tetris.cpp`, `Tetris/Game.h`, `Tetris/Keys.h`, `Tetris/Settings.h`.
- После изменений — сборка без ошибок и предупреждений.
- Ввод только через `GetAsyncKeyState`; консоль 80×25, unicode-режим.
- Предупреждения анализа гасить `(void)вызов()`, не `#pragma warning(disable)`.
- `using namespace` запрещён — только точечные `using`-декларации.
- `README.md` всегда держать актуальным (обновлять вместе с изменениями кода).
