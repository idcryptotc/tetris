# CLAUDE.md

Полное руководство для агентов — в [AGENTS.md](AGENTS.md). Кратко:

- Консольный Тетрис на C++ (MSVC v145, Win32/x64, C++23), без внешних зависимостей.
- Код: `Tetris/Main.cpp`, `Tetris/Core/Tetris.h`/`Tetris.cpp`, `Tetris/Core/Game.h`/`Game.cpp`, `Tetris/Core/Figure.h`/`Figure.cpp`, `Tetris/Core/FigureGenerator.h`/`FigureGenerator.cpp`, `Tetris/Core/GameData.h`, `Tetris/Rendering/Console.h`/`Console.cpp`, `Tetris/Rendering/GameField.h`/`GameField.cpp`, `Tetris/Menu/GameMenu.h`/`GameMenu.cpp`, `Tetris/Input/Keys.h`, `Tetris/Input/Settings.h`.
- После изменений — сборка без ошибок и предупреждений.
- Ввод только через `GetAsyncKeyState`; консоль 80×25, unicode-режим.
- Предупреждения анализа гасить `(void)вызов()`, не `#pragma warning(disable)`.
- `using namespace` запрещён — только точечные `using`-декларации.
- `README.md` всегда держать актуальным (обновлять вместе с изменениями кода).
- Смена кодировки файлов (конвертация, снятие BOM) — только через PowerShell; `write_file`/`replace_file_content` пишут в 1251 и ломают эмодзи/box-drawing. Если PowerShell недоступен — попросить пользователя.
