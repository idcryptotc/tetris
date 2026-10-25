#include "Console.h"
#include "GameField.h"
#include "../Core/GameData.h"

#include <string>
#include <utility>
#include <vector>

#include <conio.h>
#include <windows.h>

namespace Tetris
{
    // Шрифт 5x7 для букв логотипа. 1 — заполненная ячейка, 0 — пустая.
    // Порядок букв: T, E, R, I, S.
    static constexpr unsigned char kLogoFont[5][7][5] =
    {
        // T
        {
            { 1,1,1,1,1 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
        },
        // E
        {
            { 1,1,1,1,1 },
            { 1,0,0,0,0 },
            { 1,0,0,0,0 },
            { 1,1,1,1,0 },
            { 1,0,0,0,0 },
            { 1,0,0,0,0 },
            { 1,1,1,1,1 },
        },
        // R
        {
            { 1,1,1,1,0 },
            { 1,0,0,0,1 },
            { 1,0,0,0,1 },
            { 1,1,1,1,0 },
            { 1,0,1,0,0 },
            { 1,0,0,1,0 },
            { 1,0,0,0,1 },
        },
        // I
        {
            { 1,1,1,1,1 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 0,0,1,0,0 },
            { 1,1,1,1,1 },
        },
        // S
        {
            { 0,1,1,1,1 },
            { 1,0,0,0,0 },
            { 1,0,0,0,0 },
            { 0,1,1,1,0 },
            { 0,0,0,0,1 },
            { 0,0,0,0,1 },
            { 1,1,1,1,0 },
        },
    };

    // Индексы глифов в kLogoFont для букв TETRIS (вторая T переиспользует первую).
    static constexpr int kLetterGlyph[6] = { 0, 1, 0, 2, 3, 4 };

    // Цвет букв (классическая палитра тетромино).
    static constexpr Colors kLetterColors[6] =
    {
        Colors::CYAN,        // T
        Colors::LIGHTBLUE,   // E
        Colors::LIGHTGREEN,  // T
        Colors::MAGENTA,     // R
        Colors::LIGHTRED,    // I
        Colors::YELLOW,      // S
    };

    // Фигуры для ряда под логотипом: заполненные ячейки в сетке 4x2.
    // Порядок и цвета совпадают с FIGURE_COLORS.
    static constexpr int kPieceCells[7][4][2] =
    {
        { { 0,0 }, { 1,0 }, { 2,0 }, { 3,0 } }, // I
        { { 1,0 }, { 2,0 }, { 1,1 }, { 2,1 } }, // O
        { { 0,0 }, { 1,0 }, { 2,0 }, { 1,1 } }, // T
        { { 1,0 }, { 2,0 }, { 0,1 }, { 1,1 } }, // S
        { { 0,0 }, { 1,0 }, { 1,1 }, { 2,1 } }, // Z
        { { 0,0 }, { 0,1 }, { 1,1 }, { 2,1 } }, // J
        { { 2,0 }, { 0,1 }, { 1,1 }, { 2,1 } }, // L
    };

    // Рисует одну фигуру из ряда в кадре.
    static void DrawPiece(
        std::vector<std::pair<std::wstring, std::vector<int>>> &frame,
        int startX,
        int startY,
        const int cells[4][2],
        int color)
    {
        // cells — это указатель const int (*)[2], range-for по нему не работает
        // (нет begin/end), поэтому обычный цикл по 4 строкам.
        for (int i = 0; i < 4; ++i)
        {
            const int x = startX + cells[i][0];
            const int y = startY + cells[i][1];
            frame[y].first[x] = L'█';
            frame[y].second[x] = color;
        }
    }

    // Собирает кадр заставки (24 строки x 79 колонок).
    static std::vector<std::pair<std::wstring, std::vector<int>>> BuildSplashFrame()
    {
        // 24 строки — как игровой кадр GAME_FIELD (строка 25 остаётся пустой).
        // Ширина 79, а не 80: строка в 80 символов сама переносит курсор
        // на следующую строку, и ViewGameField (символы + endl) рисует её
        // в две строки — кадр уезжает за экран.
        std::vector<std::pair<std::wstring, std::vector<int>>> frame(24);
        for (auto &row : frame)
        {
            row.first.assign(79, L' ');
            row.second.assign(79, 0);
        }

        // Логотип: масштаб 2, буква 10x14, зазор 2.
        const int scale = 2;
        const int letterWidth = 5 * scale;
        const int totalWidth = 6 * letterWidth + 5 * 2;
        const int startX = (80 - totalWidth) / 2;
        const int startY = 3;

        for (int letter = 0; letter < 6; ++letter)
        {
            const int glyph = kLetterGlyph[letter];
            const int baseX = startX + letter * (letterWidth + 2);
            const int color = static_cast<int>(kLetterColors[letter]);

            for (int py = 0; py < 7; ++py)
            {
                for (int px = 0; px < 5; ++px)
                {
                    if (kLogoFont[glyph][py][px])
                    {
                        for (int sy = 0; sy < scale; ++sy)
                        {
                            for (int sx = 0; sx < scale; ++sx)
                            {
                                const int x = baseX + px * scale + sx;
                                const int y = startY + py * scale + sy;
                                frame[y].first[x] = L'█';
                                frame[y].second[x] = color;
                            }
                        }
                    }
                }
            }
        }

        // Ряд фигур под логотипом: каждая 4 колонки + зазор 2.
        const int pieceStartX = (80 - (7 * 4 + 6 * 2)) / 2;
        const int pieceStartY = 19;
        for (int piece = 0; piece < 7; ++piece)
        {
            const int color = static_cast<int>(FIGURE_COLORS.at(piece));
            DrawPiece(frame, pieceStartX + piece * 6, pieceStartY, kPieceCells[piece], color);
        }

        // Подсказка внизу.
        const std::wstring hint = L"Нажмите любую клавишу";
        const int hintX = (80 - static_cast<int>(hint.size())) / 2;
        for (int i = 0; i < static_cast<int>(hint.size()); ++i)
        {
            frame[22].first[hintX + i] = hint[i];
        }

        return frame;
    }

    // Проверка параметров окна (только адаптивный режим): если окно меньше
    // кадра 80x25 — MessageBox (гарантия видимости: строка в консоли
    // в маленьком окне может быть обрезана вьюпортом). Шрифт не проверяем —
    // в conpty его контролирует терминал, а поле корректно рисуется
    // с любым шрифтом (просто будет выглядеть плотнее/реже).
    static void CheckWindowParameters()
    {
        if (g_console.mode != ConsoleMode::Adaptive)
        {
            return;
        }

        if (g_console.csbi.dwSize.X < 80 || g_console.csbi.dwSize.Y < 25)
        {
            (void)MessageBoxW(
                nullptr,
                L"Увеличьте окно консоли хотя бы до 80x25 — поле будет обрезано",
                L"Тетрис",
                MB_OK | MB_ICONWARNING);
        }
    }

    void ShowSplash()
    {
        CheckWindowParameters();
        std::vector<std::pair<std::wstring, std::vector<int>>> frame = BuildSplashFrame();
        ViewGameField(frame);
        (void)_getch(); // ждём любую клавишу
    }
}
