# AGENTS.md — руководство для ИИ-агентов

## О проекте

Консольный **Тетрис** для Windows. Один исполняемый файл, без внешних зависимостей — только WinAPI и стандартная библиотека C++.

- Платформа: **x64** (по умолчанию) и **Win32** (x86, опционально), конфигурации Debug/Release.
- Компилятор: **MSVC, toolset v145 (Visual Studio 2026)**.
- Стандарт: **C++23** (`CMAKE_CXX_STANDARD 23` в `CMakeLists.txt`).
- Сборка: **CMake** (генератор Visual Studio 18 2026).
- **Нет** прекомпилированного заголовка (PCH удалён), **нет** тестов, **нет** CI.

## Структура

| Файл | Назначение |
|---|---|
| `Tetris/Tetris.cpp` | Игровая логика: цикл, рендер, двойная буферизация консоли, меню, ввод, `main()`. Вся логика (кроме `main()`) — в `namespace Tetris` |
| `Tetris/Figure.h` | Класс `Figure`: объявление |
| `Tetris/Figure.cpp` | Класс `Figure`: реализация, движение, поворот, состояние |
| `Tetris/Game.h` | Header-only (всё `constexpr`/`inline const`): энамы (`Actions`, `MoveDirection`, `FallResult`, `MenuResult`, `Colors`), константы геометрии, таблицы фигур/цветов/переходов, раскладка `GAME_FIELD` |
| `Tetris/Keys.h` | VK-коды клавиш (`KeyCodes`), действия (`BindingAction`), набор привязок (`KeyBindings`), опрос клавиатуры (`IsKeyPressed`) |
| `Tetris/Settings.h` | Header-only (всё `inline`): загрузка/сохранение `tetris_settings.ini`, интерактивное меню переназначения клавиш |
| `CMakeLists.txt` | Конфигурация сборки CMake (C++23, /W3, UNICODE; x64 или Win32 через `-A`) |
| `README.md` | Описание игры для пользователей |

## Сборка

Сборка только через CMake (генератор Visual Studio 18 2026, toolset v145). Архитектура задаётся флагом `-A` при генерации:

- **x64 (по умолчанию):** `cmake -B build -G "Visual Studio 18 2026" -A x64`, затем `cmake --build build --config Release` (или `Debug`).
- **x86 (Win32):** `cmake -B build-x86 -G "Visual Studio 18 2026" -A Win32`, затем `cmake --build build-x86 --config Release`.

После любого изменения кода **обязательно** пересоберите проект и убедитесь, что ошибок и предупреждений нет (анализатор MSVC включён, предупреждения — это проблемы).

> ⚠️ Если при сборке появляется **MSB8052** («версия набора инструментов MSVC не совместима с набором инструментов платформы») — это конфликт `VCToolsVersion` в пользовательских листах `%LOCALAPPDATA%\Microsoft\MSBuild\v14.0\Microsoft.Cpp.<платформа>.user.props`. Удалите из них явное свойство `<VCToolsVersion>` — тогда v145 возьмёт свой дефолтный компилятор.

## Как устроено

- **Игровой цикл:** `main()` → меню (`StartMenu`) → `PlayGame()` в цикле. Фигура падает по таймеру `LEVEL_TIME_STEP` (25 уровней), ввод опрашивается каждый шаг.
- **Рендер:** двойная буферизация через `CreateConsoleScreenBuffer`/`ReadConsoleOutput`/`WriteConsoleOutput` (структура `ConsoleRenderer g_console`). Отрисовка рисуется в резервный буфер, затем `SetConsoleActiveScreenBuffer` переключает активный.
- **Поле:** `GAME_FIELD` — 22 строки × 45 колонок (`wstring` + параллельный вектор цветов). Стек фигур `gameStack` — 22 × 21, где последний столбец `[WIDTH]` — счётчик заполнения строки (10 = строка полная, очищается).
- **Фигуры:** битовые маски 16 бит (`FIGURES`), обход по биту `shift = 1 << 15`.
- **Ввод:** опрос глобального состояния клавиатуры через `GetAsyncKeyState` (`IsKeyPressed`), **не** очередь сообщений.
- **Настройки:** `tetris_settings.ini` в рабочей директории, секция `Bindings`, ключи `Left/Right/RotateUp/RotateDown/HardDrop/Exit`. Записываются через `WritePrivateProfileStringW`, читаются через `GetPrivateProfileStringW`.

## Критичные инварианты (не ломать)

- Консоль **80×25**, шрифт высотой **24** — всё поле нарисовано под эти размеры (`mode con cols=80 lines=25`, `CHAR_INFO buffer[2000]`).
- Консоль работает в **unicode-режиме** (`_setmode(_fileno(stdout), _O_U16TEXT)`, весь вывод через `std::wcout`, строки `L"..."`). Не использовать `printf`/`cout` для вывода в игру.
- **Не использовать** `PeekMessageW`/`GetMessageW` — в консольном приложении они виснут. Ввод только через `GetAsyncKeyState` + `_kbhit`/`_getch`.
- **Не использовать** `ReadConsoleInputW` — в свежем SDK структура `INPUT_RECORD` нестандартна.
- Порядок приоритета клавиш в игровом цикле: LEFT > RIGHT > ROTATE_UP > ROTATE_DOWN > HARD_DROP > EXIT.
- Порядок элементов `BindingAction` совпадает с порядком `KeyBindings::keys` и `kBindingKeys` — при добавлении действия менять все три места.

## Стиль кода

Полный стиль кода — в [CODE_STYLE.md](CODE_STYLE.md) (он же основной документ для людей). Кратко:

- Комментарии и идентификаторы-подписи — **на русском** (UI-строки тоже).
- Скобки Allman (открывающая скобка с новой строки), отступ 4 пробела.
- Именование: функции и методы — `PascalCase` (`CreateGameField`, `SetPosition`); локальные переменные — `camelCase` (`idFigure`); константы — `UPPER_SNAKE_CASE` (`MAX_FALL_LINES`); глобальные — префикс `g_` (`g_console`); константные данные в заголовках — префикс `k` (`kSettingsFileName`).
- Каждый файл включает **то, что использует сам** (PCH нет). `std::size_t` требует явного `#include <cstddef>`.
- **`using namespace` запрещён** — только точечные `using`-декларации (по одному символу на имя).
- Нежелательные предупреждения анализа (например, C6031 «возвращаемое значение пропущено») гасить **явным приведением `(void)вызов()`** с комментарием, а не `#pragma warning(disable: ...)`.
- Заголовки `Game.h`/`Keys.h`/`Settings.h` — header-only: всё, что в них объявлено, должно быть `inline`/`constexpr`/`inline const`.

### Форматирование: два конфига, две роли

В корне репозитория лежат **два** файла конфигурации форматирования. Они не конкурируют — у них разные зоны ответственности:

| Файл | Отвечает за |
|---|---|
| `.editorconfig` | Универсальные свойства файла и редактора: `charset`, `end_of_line`, `insert_final_newline`, `trim_trailing_whitespace`, базовые `indent_style`/`indent_size`, а также настройки для **не-C++** файлов (`.ini`, `.md`). |
| `.clang-format` | **Единственный источник истины** для форматирования C/C++ кода: стиль скобок (Allman), отступ 4, `ColumnLimit`, выравнивание `case`, группировка `#include` и т.п. |

Правила:

- Для C/C++ при конфликте правил приоритет у **`.clang-format`**. C++-специфичные ключи `cpp_*` в `.editorconfig` **не дублируют** `.clang-format` и не должны появляться.
- `indent_style`/`indent_size` в `.editorconfig` обязаны быть согласованы с `IndentWidth`/`UseTab` в `.clang-format`. Если разошлись — это баг, сообщить, а не выбирать самому.
- Не-C++ файлы (`.ini`, `README.md`) форматируются по `.editorconfig`, `.clang-format` к ним не применяется.
- Третий файл конфигурации (`.clang-tidy`, `.cmake-format` и т.п.) не добавлять без явного запроса.

## Working with git in the agent environment (slow cold-start workaround)

**Problem.** The first `git.exe` launch in a session takes 30-45 seconds (Defender/AV, cold cache). The MCP request timeout in VS is 30 000 ms, so calling a git command directly through the agent tools (`run_powershell`, `git_status`, `git_log`, etc.) fails with `MCP request N timed out after 30000ms` - even when the tool `timeout` parameter is set higher.

**Workaround.** Run the git command with its output redirected to a file in the working folder. The MCP request still times out, but the background PowerShell process keeps running and writes the file; then read the file.

```powershell
# 1) Launch: the MCP request will time out - that is normal, the process keeps living
powershell -NoProfile -Command "Set-Location -LiteralPath 'D:\__ShiftDelete\github\tetris'; cmd /c 'git status > _gs_out.txt 2>&1'"
# 2) Read the result: read_file _gs_out.txt
```

**Output encoding.** Redirecting through PowerShell `>` writes **UTF-16LE** (with null bytes) - reading it as UTF-8 makes the text fall apart. To get plain text, use `cmd /c 'git ... > file 2>&1'` (console encoding) or set `[Console]::OutputEncoding = [System.Text.Encoding]::UTF8` explicitly.

### Polling protocol for background git commands

When a git command is launched in the background (a `.bat` with output redirected to files), poll for the result file as follows:

1. Launch the `.bat`. The MCP request times out at ~30 s, but the `.bat` survives and keeps running; by the time the timeout returns, ~30 s of real time have already passed.
2. Read the result file immediately - this is the "check after 30 s" step, no extra sleep needed.
3. If the file is absent, sleep and re-read every ~10 s, up to 10 times.
4. If still absent, do one final check after ~30 s.
5. If still absent, report: "Mr.Git отказываетесь с нами играть".
6. If the file appears but is empty or contains an error, report: "Mr.Git нарушает все правила!".

Note: each sleep must stay below the 30 s MCP request timeout, so in practice use ~9 s (step 3) and ~25 s (step 4).

## Правила для агентов

1. Не возвращать PCH/`stdafx.h` — проект намеренно без прекомпилированного заголовка.
2. Не добавлять внешние зависимости без явного запроса (дорожная карта в README упоминает miniaudio/XInput — только если пользователь просит).
3. Форматирование C++ — только по `.clang-format`; `.editorconfig` не трогать для C++-правил. Подробнее — в разделе «Стиль кода → Форматирование».
4. После изменений: сборка без ошибок **и** без предупреждений.
5. `README.md` **всегда должен быть актуален**: при изменении функциональности, структуры файлов, управления или технологий обновляйте соответствующие разделы README в том же изменении.
6. При правке `README.md` сохранять кодировку UTF-8 (в файле есть эмодзи, стрелки и box-drawing символы).
7. Не коммитить артефакты: `build/`, `build-x86/`, `Debug/`, `Release/`, `.vs/`.
8. Комментарий к коммиту — на русском, краткий, в одну строку.
9. Кириллицу, собранную из кодовых точек, Всегда читать обратно через `read_file` и визуально сверять до использования.
Справочник: [`cyrillic.md`](cyrillic.md).
10. `push` — строго только по разрешению.
11. Данный проект не имеет решения, поэтому VS-инструменты файлов не работают, delete_file не работает - для работы с файлами используй PowerShell или cmd.
