# AGENTS.md — руководство для ИИ-агентов

## О проекте

Консольный **Тетрис** для Windows. Один исполняемый файл, без внешних зависимостей — только WinAPI и стандартная библиотека C++.

- Платформы: **Win32** (x86) и **x64**, конфигурации Debug/Release.
- Компилятор: **MSVC, toolset v145 (Visual Studio 18)**.
- Стандарт: **C++23** (`<LanguageStandard>stdcpp23</LanguageStandard>` в vcxproj).
- Кодировка исходников: **UTF-8 без BOM**, компилятору задан флаг `/utf-8` (`<AdditionalOptions>` в vcxproj) — сборка не зависит от кодовой страницы системы.
- **Нет** прекомпилированного заголовка (PCH удалён), **нет** тестов, **нет** CI.

## Структура

```
Tetris/
├─ Main.cpp              ← точка входа
├─ Core/                 ← игровая логика: Tetris.h/.cpp, Game.h/.cpp,
│                          Figure.h/.cpp, FigureGenerator.h/.cpp, GameData.h
├─ Rendering/            ← рендер: Console.h/.cpp, GameField.h/.cpp, Splash.h/.cpp,
│                          GameEnd.h/.cpp, PixelFont.h
├─ Input/                ← ввод и настройки: Keys.h, Settings.h
└─ Menu/                 ← меню: GameMenu.h/.cpp
```

| Файл | Назначение |
|---|---|
| `Tetris/Main.cpp` | Точка входа: `main()` — инициализация, загрузка настроек, заставка, цикл меню/партий. Использует публичные функции из `Core/Tetris.h`, `Rendering/Console.h`, `Rendering/Splash.h`, `Menu/GameMenu.h` и `Input/Settings.h` |
| `Tetris/Core/Tetris.h` / `Tetris/Core/Tetris.cpp` | Игровой цикл, ввод. Вся логика — в `namespace Tetris`; `Tetris.h` — объявление `PlayGame()` (единственная функция, доступная извне) |
| `Tetris/Rendering/Console.h` / `Tetris/Rendering/Console.cpp` | Консоль: `ProgramSettings()` (инициализация), двойная буферизация `DoubleBufferingBegin/End`, `g_console` (`ConsoleRenderer`) |
| `Tetris/Core/Game.h` / `Tetris/Core/Game.cpp` | Правила игры: столкновения, движения, падение, фиксация, очистка линий, счёт и уровни |
| `Tetris/Rendering/GameField.h` / `Tetris/Rendering/GameField.cpp` | Рендер поля: `CreateGameField` (сборка кадра), `ViewGameField` (вывод в консоль) |
| `Tetris/Rendering/Splash.h` / `Tetris/Rendering/Splash.cpp` | Заставка при запуске: логотип «TETRIS» + ряд фигур, висит до нажатия любой клавиши (`ShowSplash`) |
| `Tetris/Rendering/GameEnd.h` / `Tetris/Rendering/GameEnd.cpp` | Экраны конца партии: «GAME OVER» (`ShowGameOver`) и «YOU WIN» (`ShowWin`) — пиксельное сообщение + счёт/уровень, ожидание Enter |
| `Tetris/Rendering/PixelFont.h` | Пиксельный шрифт 5×7 (26 букв латиницы) — логотип заставки и экраны конца партии |
| `Tetris/Menu/GameMenu.h` / `Tetris/Menu/GameMenu.cpp` | Меню: `StartMenu` (главное меню), `ReturnToMenu` (возврат из игры), `ClearKeyboardBuffer` (очистка буфера клавиатуры) |
| `Tetris/Core/Figure.h` / `Tetris/Core/Figure.cpp` | Класс `Figure` — падающая фигура: позиция, поворот, сдвиги, применение действия |
| `Tetris/Core/FigureGenerator.h` / `Tetris/Core/FigureGenerator.cpp` | Генерация фигур: `RandomFigureId` (случайный id), `PositionById` (случайная стартовая позиция) |
| `Tetris/Core/GameData.h` | Header-only (всё `constexpr`/`inline const`): энамы (`Actions`, `MoveDirection`, `FallResult`, `MenuResult`, `Colors`), константы геометрии, таблицы фигур/цветов/переходов, раскладка `GAME_FIELD` |
| `Tetris/Input/Keys.h` | VK-коды клавиш (`KeyCodes`), действия (`BindingAction`), набор привязок (`KeyBindings`), опрос клавиатуры (`IsKeyPressed`) |
| `Tetris/Input/Settings.h` | Header-only (всё `inline`): загрузка/сохранение `tetris_settings.ini`, интерактивное меню переназначения клавиш |
| `Tetris/Tetris.vcxproj` | Проект MSBuild (Win32/x64, v145) |
| `README.md` | Описание игры для пользователей |
| `Plan.md` | Планы и «отложенные» идеи: блоки по приоритету — в работе / отложенные / оставлено про запас / завершённые (указатели на `DevDocs/`) |
| `DevDocs/` | Документация по завершённым задачам (один документ на задачу: проблема → решение → почему → что проверено → ограничения) |

## Сборка

Сборка только через Visual Studio 18 (toolset v145) (или `msbuild Tetris.slnx /p:Configuration=Debug /p:Platform=Win32`).
После любого изменения кода **обязательно** пересоберите проект и убедитесь, что ошибок и предупреждений нет (анализатор MSVC включён, предупреждения — это проблемы).

## Как устроено

- **Игровой цикл:** `main()` (`Main.cpp`) → заставка (`ShowSplash`) → меню (`StartMenu`) → `PlayGame()` в цикле. Фигура падает по таймеру `LEVEL_TIME_STEP` (25 уровней), ввод опрашивается каждый шаг.
- **Рендер:** два режима (`ConsoleMode`, `Console.h`/`Console.cpp`), оба с двойной буферизацией (структура `ConsoleRenderer g_console`): кадр рисуется в скрытый `basicBuffer` (на него пишет `wcout` независимо от активного буфера), предыдущий кадр показывается в резервном `doubleBuffer`, затем `SetConsoleActiveScreenBuffer(basicBuffer)` атомарно переключает. **Fixed** — 16-цвет, предыдущий кадр копируется через `ReadConsoleOutput`/`WriteConsoleOutput` (CHAR_INFO). **Adaptive** (conpty-хосты) — кадр 80×25 центрируется в реальном буфере (`frameOffset`); truecolor-кадр (заставка) показывается в `doubleBuffer` через `WriteConsoleW` с сохранённой VT-строкой (`prevFrame`), т.к. CHAR_INFO теряет 24-бит; VT включён на обоих буферах. Подробности и обоснование — `DevDocs/adaptive-rendering.md`.
- **Поле:** `GAME_FIELD` — 22 строки × 45 колонок (`wstring` + параллельный вектор цветов). Стек фигур `gameStack` — 22 × 21, где последний столбец `[WIDTH]` — счётчик заполнения строки (10 = строка полная, очищается).
- **Фигуры:** битовые маски 16 бит (`FIGURES`), обход по биту `shift = 1 << 15`.
- **Ввод:** опрос глобального состояния клавиатуры через `GetAsyncKeyState` (`IsKeyPressed`), **не** очередь сообщений.
- **Настройки:** `tetris_settings.ini` в рабочей директории, секция `Bindings`, ключи `Left/Right/RotateUp/RotateDown/HardDrop/Exit`. Записываются через `WritePrivateProfileStringW`, читаются через `GetPrivateProfileStringW`.

## Критичные инварианты (не ломать)

- Кадр **80×25**, шрифт высотой **24** — всё поле нарисовано под эти размеры (`CHAR_INFO buffer[2000]`). Два режима (`ConsoleMode`), оба с двойной буферизацией: кадр рисуется в скрытый `basicBuffer` (на него пишет `wcout` независимо от активного буфера), предыдущий кадр показывается в `doubleBuffer`, затем `SetConsoleActiveScreenBuffer(basicBuffer)` атомарно переключает. **Fixed** — классическая консоль форсирует 80×25 через `mode con cols=80 lines=25` и шрифт 24px; **Adaptive** — conpty-хост (Windows Terminal и т.п.; детект: `WT_SESSION` → `TERM_PROGRAM` → класс окна `PseudoConsoleWindow`, т.к. при запуске с повышенными правами переменные не пробрасываются), размер и шрифт не трогаем (управляет терминал; принудительная смена шрифта в conpty сбрасывает буфер к 80×25), буфер следует за окном, кадр центрируется в реальном буфере (`ViewGameField`, смещение `g_console.frameOffset`), truecolor-кадр показывается в `doubleBuffer` через `WriteConsoleW` с сохранённой VT-строкой (`prevFrame`), при окне < 80×25 — `MessageBoxW` (`CheckWindowParameters`).
- Консоль работает в **unicode-режиме** (`_setmode(_fileno(stdout), _O_U16TEXT)`, весь вывод через `std::wcout`, строки `L"..."`). Не использовать `printf`/`cout` для вывода в игру.
- **Не использовать** `PeekMessageW`/`GetMessageW` — в консольном приложении они виснут. Ввод только через `GetAsyncKeyState` + `_kbhit`/`_getch`.
- **Не использовать** `ReadConsoleInputW` — в свежем SDK структура `INPUT_RECORD` нестандартна.
- Если в игровом цикле нажато несколько клавиш одновременно, обрабатывается только первая в порядке: LEFT, RIGHT, ROTATE_UP, ROTATE_DOWN, HARD_DROP, EXIT.
- Порядок элементов `BindingAction` совпадает с порядком `KeyBindings::keys` и `kBindingKeys` — при добавлении действия менять все три места.

## Стиль кода

Полный стиль кода — в [CODE_STYLE.md](CODE_STYLE.md) (он же основной документ для людей). Кратко:

- Комментарии и идентификаторы-подписи — **на русском** (UI-строки тоже).
- Скобки Allman (открывающая скобка с новой строки), отступ 4 пробела.
- Именование: функции и методы — `PascalCase` (`CreateGameField`, `SetPosition`); локальные переменные — `camelCase` (`idFigure`); константы — `UPPER_SNAKE_CASE` (`MAX_FALL_LINES`); глобальные — префикс `g_` (`g_console`); константные данные в заголовках — префикс `k` (`kSettingsFileName`).
- Каждый файл включает **то, что использует сам** (PCH нет). `std::size_t` требует явного `#include <cstddef>`.
- **`using namespace` запрещён** — только точечные `using`-декларации (по одному символу на имя).
- Нежелательные предупреждения анализа (например, C6031 «возвращаемое значение пропущено») гасить **явным приведением `(void)вызов()`** с комментарием, а не `#pragma warning(disable: ...)`.
- Заголовки `GameData.h`/`Keys.h`/`Settings.h`/`PixelFont.h` — header-only: всё, что в них объявлено, должно быть `inline`/`constexpr`/`inline const`. `Figure.h`, `Console.h`, `Game.h`, `GameField.h`, `GameMenu.h`, `GameEnd.h` и `FigureGenerator.h` — обычные заголовки с определениями в `Figure.cpp`/`Console.cpp`/`Game.cpp`/`GameField.cpp`/`GameMenu.cpp`/`GameEnd.cpp`/`FigureGenerator.cpp`. `Tetris.h` — только объявление публичной функции, определение — в `Tetris.cpp`.

### Форматирование: два конфига, две роли

В корне репозитория лежат **два** файла конфигурации форматирования. Они не конкурируют — у них разные зоны ответственности:

| Файл | Отвечает за |
|---|---|
| `.editorconfig` | Универсальные свойства файла и редактора: `charset`, `end_of_line`, `insert_final_newline`, `trim_trailing_whitespace`, базовые `indent_style`/`indent_size`, а также настройки для **не-C++** файлов (`.vcxproj`, `.slnx`, `.ini`, `.md`). |
| `.clang-format` | **Единственный источник истины** для форматирования C/C++ кода: стиль скобок (Allman), отступ 4, `ColumnLimit`, выравнивание `case`, группировка `#include` и т.п. |

Правила:

- Для C/C++ при конфликте правил приоритет у **`.clang-format`**. C++-специфичные ключи `cpp_*` в `.editorconfig` **не дублируют** `.clang-format` и не должны появляться.
- `indent_style`/`indent_size` в `.editorconfig` обязаны быть согласованы с `IndentWidth`/`UseTab` в `.clang-format`. Если разошлись — это баг, сообщить, а не выбирать самому.
- Не-C++ файлы (`.vcxproj`, `.slnx`, `.ini`, `README.md`) форматируются по `.editorconfig`, `.clang-format` к ним не применяется.
- Третий файл конфигурации (`.clang-tidy`, `.cmake-format` и т.п.) не добавлять без явного запроса.

## Правила для агентов

1. Не возвращать PCH/`stdafx.h` — проект намеренно без прекомпилированного заголовка.
2. Не добавлять внешние зависимости без явного запроса (дорожная карта в README упоминает miniaudio/XInput — только если пользователь просит).
3. Форматирование C++ — только по `.clang-format`; `.editorconfig` не трогать для C++-правил. Подробнее — в разделе «Стиль кода → Форматирование».
4. После изменений: сборка без ошибок **и** без предупреждений.
5. `README.md` **всегда должен быть актуален**: при изменении функциональности, структуры файлов, управления или технологий обновляйте соответствующие разделы README в том же изменении.
6. При правке `README.md` сохранять кодировку UTF-8 (в файле есть эмодзи, стрелки и box-drawing символы). **Смену кодировки** (конвертация 1251 → UTF-8, снятие BOM, проверка байтов) делать **через PowerShell** (`run_powershell`): инструменты записи `write_file`/`replace_file_content` пишут в системной кодировке (1251) и **ломают** эмодзи/box-drawing (превращают в `?`). Если PowerShell недоступен — **попросить пользователя** выполнить команду вручную.
7. Не коммитить артефакты: `Debug/`, `Release/`, `*.suo`, `*.sdf`, `.vs/`.
8. Если сборка неуспешна по неявным причинам (нет вывода сборки, пустой список ошибок) — **не повторять** попытку с другими параметрами, это не поможет: скорее всего, есть ошибки, которые не видны агенту. Звать пользователя.
9. `Plan.md` в корне решения — место для «отложенных» мыслей и идей, пока не реализованных. Блоки по приоритету: **1. В работе** → **2. Отложенные** → **3. Оставлено про запас (диагностика)** → **4. Завершённые**. Перед работой по отложенной идее сверяйтесь с этим файлом; новые отложенные мысли записывайте в блок 2. Завершённую задачу выносите в `DevDocs/` — один документ на задачу (шаблон: `DevDocs/adaptive-rendering.md`: статус/коммит → проблема → решение → ключевые решения с обоснованием → что проверено → ограничения); в блоке 4 остаётся однострочный указатель. Блок 4 не раздувать: если вырастет за ~10 строк — вычищаем старейшие (детали и так в git и `DevDocs/`). Док пишется, только если есть неочевидные решения; мелкие правки — только коммит-сообщение.