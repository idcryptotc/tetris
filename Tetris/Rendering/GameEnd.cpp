#include "Console.h"
#include "GameEnd.h"
#include "GameField.h"
#include "PixelFont.h"
#include "../Core/GameData.h"
#include "../Input/Keys.h"

#include <string>
#include <utility>
#include <vector>

#include <conio.h>
#include <windows.h>

namespace Tetris
{
    // Индексы глифов (A=0, B=1, ...) для букв в «GAME OVER» и «YOU WIN».
    // Пространство между словами — пропуск (glyph = -1).
    static constexpr int kGameOverGlyphs[9] = { 6, 0, 12, 4, -1, 14, 21, 4, 17 };
    static constexpr int kWinGlyphs[7] = { 24, 14, 20, -1, 22, 8, 13 };

    // Цвет букв (классическая палитра тетромино, как в логотипе заставки).
    // Индексы совпадают с kGameOverGlyphs / kWinGlyphs.
    static constexpr Colors kGameOverColors[9] =
    {
        Colors::CYAN,         // G
        Colors::LIGHTBLUE,    // A
        Colors::LIGHTGREEN,   // M
        Colors::MAGENTA,      // E
        Colors::LIGHTGRAY,    // (пробел)
        Colors::LIGHTRED,     // O
        Colors::YELLOW,       // V
        Colors::LIGHTMAGENTA, // E
        Colors::LIGHTCYAN,    // R
    };

    static constexpr Colors kWinColors[7] =
    {
        Colors::YELLOW,     // Y
        Colors::LIGHTGREEN, // O
        Colors::LIGHTCYAN,  // U
        Colors::LIGHTGRAY,  // (пробел)
        Colors::LIGHTBLUE,  // W
        Colors::LIGHTRED,   // I
        Colors::CYAN,       // N
    };

    // Раскладка кадра: сообщение центрируется в верхней части,
    // счёт/уровень/подсказка — внизу.
    static constexpr int SCORE_ROW = 18;
    static constexpr int LEVEL_ROW = 20;
    static constexpr int HINT_ROW = 22;

    // Рисует одно пиксельное слово в кадре: буква kGlyphWidth x kGlyphHeight,
    // масштаб scale, зазор между буквами 2. Пропуск (glyph = -1) — просто
    // сдвигает курсор, ничего не рисуя.
    static void DrawPixelWord(
        std::vector<std::pair<std::wstring, std::vector<int>>> &frame,
        int startX,
        int startY,
        const int *glyphs,
        const Colors *colors,
        int count,
        int scale)
    {
        const int letterWidth = kGlyphWidth * scale;
        int x = startX;

        for (int i = 0; i < count; ++i)
        {
            if (glyphs[i] >= 0)
            {
                const int glyph = glyphs[i];
                const int color = static_cast<int>(colors[i]);

                for (int py = 0; py < kGlyphHeight; ++py)
                {
                    for (int px = 0; px < kGlyphWidth; ++px)
                    {
                        if (kPixelGlyphs[glyph][py][px])
                        {
                            for (int sy = 0; sy < scale; ++sy)
                            {
                                for (int sx = 0; sx < scale; ++sx)
                                {
                                    const int fx = x + px * scale + sx;
                                    const int fy = startY + py * scale + sy;
                                    frame[fy].first[fx] = L'█';
                                    frame[fy].second[fx] = color;
                                }
                            }
                        }
                    }
                }
            }

            x += letterWidth + 2;
        }
    }

    // Собирает кадр экрана конца партии (24 строки x 79 колонок, как заставка).
    // Ширина 79, а не 80: строка в 80 символов сама переносит курсор на
    // следующую строку, и ViewGameField рисует её в две строки — кадр уезжает.
    static std::vector<std::pair<std::wstring, std::vector<int>>> BuildEndFrame(
        const int *glyphs,
        const Colors *colors,
        int glyphCount,
        unsigned int score,
        int level,
        int scale)
    {
        std::vector<std::pair<std::wstring, std::vector<int>>> frame(24);
        for (auto &row : frame)
        {
            row.first.assign(79, L' ');
            row.second.assign(79, 0);
        }

        // Пиксельное сообщение: центрируется по горизонтали и в верхней части
        // кадра (над счётом).
        const int letterWidth = kGlyphWidth * scale;
        const int totalWidth = glyphCount * letterWidth + (glyphCount - 1) * 2;
        const int startX = (80 - totalWidth) / 2;
        const int messageHeight = kGlyphHeight * scale;
        const int startY = (SCORE_ROW - messageHeight) / 2;
        DrawPixelWord(frame, startX, startY, glyphs, colors, glyphCount, scale);

        // Счёт и уровень (не раскрашены — LIGHTGRAY, как в игровом кадре).
        const std::wstring scoreLine = L"Score " + std::to_wstring(score);
        const int scoreX = (80 - static_cast<int>(scoreLine.size())) / 2;
        for (int i = 0; i < static_cast<int>(scoreLine.size()); ++i)
        {
            frame[SCORE_ROW].first[scoreX + i] = scoreLine[i];
        }

        const std::wstring levelLine = L"Level " + std::to_wstring(level);
        const int levelX = (80 - static_cast<int>(levelLine.size())) / 2;
        for (int i = 0; i < static_cast<int>(levelLine.size()); ++i)
        {
            frame[LEVEL_ROW].first[levelX + i] = levelLine[i];
        }

        // Подсказка внизу. Enter выбран как клавиша выхода: в момент конца
        // партии идёт «отчаянная битва» — любые игровые клавиши (←, →,
        // поворот, hard drop) могут быть нажаты/забуференны и мгновенно
        // закроют экран, а Enter в игре не участвует.
        const std::wstring hint = L"Для выхода в меню нажми Enter";
        const int hintX = (80 - static_cast<int>(hint.size())) / 2;
        for (int i = 0; i < static_cast<int>(hint.size()); ++i)
        {
            frame[HINT_ROW].first[hintX + i] = hint[i];
        }

        return frame;
    }

    // Общий вывод: кадр + ожидание Enter.
    // Сначала сбрасываем буфер клавиатуры: в момент конца партии идёт
    // «отчаянная битва» — нажатия игровых клавиш (←, →, поворот, hard drop)
    // ещё могут быть в буфере и мгновенно закроют экран. Enter в игре не
    // участвует, поэтому ждём именно его.
    static void ShowEndScreen(
        const int *glyphs,
        const Colors *colors,
        int glyphCount,
        unsigned int score,
        int level,
        int scale)
    {
        std::vector<std::pair<std::wstring, std::vector<int>>> frame =
            BuildEndFrame(glyphs, colors, glyphCount, score, level, scale);
        ViewGameField(frame, true);

        // Очистка буфера от «битвы».
        while (_kbhit())
        {
            (void)_getch();
        }

        // Ждём Enter (13).
        while (_getch() != static_cast<int>(KeyCodes::ENTER))
        {
        }
    }

    void ShowGameOver(unsigned int score, int level)
    {
        // 9 слотов (8 букв + пробел) не помещаются в 79 колонок при
        // масштабе 2 (106 > 79), поэтому «GAME OVER» рисуется в масштабе 1.
        ShowEndScreen(kGameOverGlyphs, kGameOverColors, 9, score, level, 1);
    }

    void ShowWin(unsigned int score, int level)
    {
        // 7 слотов (6 букв + пробел) не помещаются в 79 колонок при
        // масштабе 2 (82 > 79), поэтому «YOU WIN» рисуется в масштабе 1 —
        // как и «GAME OVER» (визуальное единообразие).
        ShowEndScreen(kWinGlyphs, kWinColors, 7, score, level, 1);
    }
}
