#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Tetris
{
    // Действия игрового цикла (при одновременном нажатии нескольких клавиш
    // обрабатывается первая в порядке: LEFT, RIGHT, ROTATE_UP, ROTATE_DOWN, HARD_DROP, EXIT).
    enum class Actions
    {
        LEFT, RIGHT, DOWN, ROTATE, DEFAULT
    };

    // Направление «пробного» движения фигуры (для проверки столкновений).
    enum class MoveDirection
    {
        Left, Right, RotateUp, RotateDown, Drop
    };

    // Результат сдвига фигуры на ряд вниз.
    enum class FallResult
    {
        Fell,   // фигура упала на ряд
        Locked  // фигура зафиксирована в стеке
    };

    // Результат меню «возврат в меню» (значения совпадают с пунктами меню).
    enum class MenuResult
    {
        Restart = 1, // Новая игра
        Stay = 2, // Продолжить
        Quit = 3  // Выход
    };

    // Палитра цветов консоли (16 цветов).
    enum class Colors
    {
        BLACK,
        BLUE,
        GREEN,
        CYAN,
        RED,
        MAGENTA,
        BROWN,
        LIGHTGRAY,
        DARKGRAY,
        LIGHTBLUE,
        LIGHTGREEN,
        LIGHTCYAN,
        LIGHTRED,
        LIGHTMAGENTA,
        YELLOW,
        WHITE
    };

    // Ряды падения фигуры: 0..LAST_FALL_LINE (всего MAX_FALL_LINES).
    constexpr int MAX_FALL_LINES = 19;
    constexpr int LAST_FALL_LINE = MAX_FALL_LINES - 1;

    // Геометрия фигуры и поля: шаг сдвига 2, квадрат 4x4, поле WIDTH x HEIGHT.
    constexpr int SHIFT_MULTIPLIER = 2;
    constexpr int SIDE_SQUARE = 4;
    constexpr int WIDTH = 10 * SHIFT_MULTIPLIER, HEIGHT = 22;
    constexpr int BASIC_X = 12, BASIC_Y = 0;

    // Параметры уровней: время падения (мс) и пороги счёта.
    inline const std::vector<unsigned int> LEVEL_TIME_STEP
    {
        500,480,460,440,420,
        400,380,360,340,320,
        300,280,260,240,220,
        200,180,160,140,120,
        100,80,60,40,20
    };

    inline const std::vector<unsigned int> LEVEL_SCORE
    {
        0,50,100,150,200,
        300,400,500,600,700,
        900,1100,1300,1500,1700,
        2000,2300,2600,2900,3200,
        4000,5000,6000,7000,8000
    };

    // Координаты и раскладка UI.
    constexpr int COUNT_STEP_CHAPTER = 8;
    constexpr int NEXT_X1 = 31, NEXT_X2 = 38, NEXT_Y1 = 1, NEXT_Y2 = 4;
    constexpr int CURRENT_X1 = 6, CURRENT_X2 = 25, CURRENT_Y1 = 3, CURRENT_Y2 = 21;
    constexpr int SCORE_X = 40, SCORE_Y = 7;
    constexpr int LEVEL_X = 40, LEVEL_Y = 9;

    // Битовые маски фигур (16 бит): 7 фигур, у каждой несколько состояний поворота.
    inline const std::vector<std::vector<unsigned short>> FIGURES =
    {
        { 0x088c,0x0446,0x0223,0x002e,0x0017,0x0c44,0x0622,0x0311,0x00e8,0x0074 },
        { 0x044c,0x0226,0x0113,0x008e,0x0047,0x0c88,0x0644,0x0322,0x00e2,0x0071 },
        { 0x08c8,0x0464,0x0232,0x004e,0x0027,0x04c4,0x0262,0x0131,0x00e4,0x0072 },
        { 0x08c4,0x0462,0x0231,0x006c,0x0036 },
        { 0x04c8,0x0264,0x0132,0x00c6,0x0063 },
        { 0x8888,0x4444,0x2222,0x1111,0x000f },
        { 0x00cc,0x0066,0x0033 }
    };

    // Цвета фигур по id.
    inline const std::map<int, Colors> FIGURE_COLORS
    {
        { 0,Colors::CYAN },
        { 1,Colors::LIGHTBLUE },
        { 2,Colors::LIGHTGREEN },
        { 3,Colors::MAGENTA },
        { 4,Colors::LIGHTRED },
        { 5,Colors::YELLOW },
        { 6,Colors::DARKGRAY }
    };

    // Переходы позиций: сдвиг влево.
    inline const std::vector<std::map<int, int>> NEXT_POSITION_LEFT
    {
        { { 1,0 },{ 2,1 },{ 4,3 },{ 6,5 },{ 7,6 },{ 9,8 } },
        { { 1,0 },{ 2,1 },{ 4,3 },{ 6,5 },{ 7,6 },{ 9,8 } },
        { { 1,0 },{ 2,1 },{ 4,3 },{ 6,5 },{ 7,6 },{ 9,8 } },
        { { 1,0 },{ 2,1 },{ 4,3 } },
        { { 1,0 },{ 2,1 },{ 4,3 } },
        { { 1,0 },{ 2,1 },{ 3,2 } },
        { { 1,0 },{ 2,1 } }
    };

    // Переходы позиций: сдвиг вправо.
    inline const std::vector<std::map<int, int>> NEXT_POSITION_RIGHT
    {
        { { 0,1 },{ 1,2 },{ 3,4 },{ 5,6 },{ 6,7 },{ 8,9 } },
        { { 0,1 },{ 1,2 },{ 3,4 },{ 5,6 },{ 6,7 },{ 8,9 } },
        { { 0,1 },{ 1,2 },{ 3,4 },{ 5,6 },{ 6,7 },{ 8,9 } },
        { { 0,1 },{ 1,2 },{ 3,4 } },
        { { 0,1 },{ 1,2 },{ 3,4 } },
        { { 0,1 },{ 1,2 },{ 2,3 } },
        { { 0,1 },{ 1,2 } }
    };

    // Поворот против часовой стрелки.
    inline const std::vector<std::map<int, int>> NEXT_POSITION_ROTATE_UP
    {
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,5 },{ 4,7 },{ 5,8 },{ 6,8 },{ 7,9 },{ 8,0 },{ 9,2 } },
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,5 },{ 4,7 },{ 5,8 },{ 6,8 },{ 7,9 },{ 8,0 },{ 9,2 } },
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,5 },{ 4,7 },{ 5,8 },{ 6,8 },{ 7,9 },{ 8,0 },{ 9,2 } },
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,0 },{ 4,2 } },
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,0 },{ 4,2 } },
        { { 0,4 },{ 1,4 },{ 2,4 },{ 3,4 },{ 4,2 } },
        {}
    };

    // Поворот по часовой стрелке.
    inline const std::vector<std::map<int, int>> NEXT_POSITION_ROTATE_DOWN
    {
        { { 0,8 },{ 1,8 },{ 2,9 },{ 3,0 },{ 4,2 },{ 5,3 },{ 6,3 },{ 7,4 },{ 8,5 },{ 9,7 } },
        { { 0,8 },{ 1,8 },{ 2,9 },{ 3,0 },{ 4,2 },{ 5,3 },{ 6,3 },{ 7,4 },{ 8,5 },{ 9,7 } },
        { { 0,8 },{ 1,8 },{ 2,9 },{ 3,0 },{ 4,2 },{ 5,3 },{ 6,3 },{ 7,4 },{ 8,5 },{ 9,7 } },
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,0 },{ 4,2 } },
        { { 0,3 },{ 1,3 },{ 2,4 },{ 3,0 },{ 4,2 } },
        { { 0,4 },{ 1,4 },{ 2,4 },{ 3,4 },{ 4,0 } },
        {}
    };

    // Начальная раскладка игрового поля (строки x 45 колонок).
    inline const std::vector<std::pair<std::wstring, std::vector<int>>> GAME_FIELD
    {
        { { L"                              ┌────────┐     " }, std::vector<int>(45, 0) },
        { { L"                              │        │     " }, std::vector<int>(45, 0) },
        { { L"                              │        │     " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║   │        │     " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║   │        │     " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║   └────────┘     " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║  Score 000000    " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║  Level     01    " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║  ESC - Выход     " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║                  " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║  ↓ ↑ - Rotate    " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║ SPACE - MegaDown " }, std::vector<int>(45, 0) },
        { { L"     ║                    ║   ←  - Left      " }, std::vector<int>(45, 0) },
        { { L"     ╚════════════════════╝   →  - Right     " }, std::vector<int>(45, 0) },
        { { L"                                             " }, std::vector<int>(45, 0) },
    };
}
