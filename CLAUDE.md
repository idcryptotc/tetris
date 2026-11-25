# CLAUDE.md

Полное руководство для агентов — в [AGENTS.md](AGENTS.md). Кратко:

- Консольный Тетрис на C++ (MSVC v145, Win32/x64, C++23), без внешних зависимостей.
- Код: `Tetris/Main.cpp`, `Tetris/Core/Tetris.h`/`Tetris.cpp`, `Tetris/Core/Game.h`/`Game.cpp`, `Tetris/Core/Figure.h`/`Figure.cpp`, `Tetris/Core/FigureGenerator.h`/`FigureGenerator.cpp`, `Tetris/Core/GameData.h`, `Tetris/Rendering/Console.h`/`Console.cpp`, `Tetris/Rendering/GameField.h`/`GameField.cpp`, `Tetris/Menu/GameMenu.h`/`GameMenu.cpp`, `Tetris/Input/Keys.h`, `Tetris/Input/Settings.h`.
- После изменений — сборка без ошибок и предупреждений.
- Ввод только через `GetAsyncKeyState`; консоль 80×25, unicode-режим.
- Предупреждения анализа гасить `(void)вызов()`, не `#pragma warning(disable)`.
- `using namespace` запрещён — только точечные `using`-декларации.
- `README.md` всегда держать актуальным (обновлять вместе с изменениями кода).
- Процесс по задаче: перенести в блок «В работе» в `Plan.md` с планом по чекбоксами, выполнять шаг за шагом с отметкой по каждому шагу; последний шаг — проверка результата пользователем (подробно — AGENTS.md, правило 10).
- Смена кодировки файлов (конвертация, снятие BOM) — только через PowerShell; `write_file`/`replace_file_content` пишут в 1251 и ломают эмодзи/box-drawing. Если PowerShell недоступен — попросить пользователя.
- Скрипты агента в `Other/`: `build.ps1` (сборка `Tetris.slnx` + прогон `Tetris.Tests.exe` — правило 4), `check-encoding.ps1` (проверка UTF-8 без BOM — правило 6), `check_agent_status.ps1` (процессы `Tetris.exe`, порты 7777–7786). Скрипты — чистый ASCII (кириллица в `.ps1` ломается в PowerShell 5.1).
- Одноразовый скрипт, который может пригодиться в других задачах, — складировать в `Other/` и упомянуть в строке `Other/` в `AGENTS.md` (правило 11).
