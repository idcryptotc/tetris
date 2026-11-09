#include "Console.h"
#include "../Core/Figure.h"
#include "../Core/GameData.h"
#include "GameField.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>

namespace Tetris
{
    // Устанавливает один квадратный пиксель в кадре. Пиксель (px, py)
    // попадает в ячейку (col = px, row = py / 2): верхняя половинка, если
    // py чётная, нижняя — если нечётная. Если обе половинки заняты, ячейка
    // становится █ (цвет — от верхней половинки, у ячейки один цвет).
    // Общее место для пиксельного рендера: заставка (Splash.cpp) и экраны
    // конца партии (GameEnd.cpp) рисуют квадратные пиксели через него.
    void SetPixel(
        std::vector<std::pair<std::wstring, std::vector<int>>> &frame,
        int px,
        int py,
        int color)
    {
        const int row = py / 2;
        const int col = px;

        wchar_t &ch = frame[row].first[col];
        int &cellColor = frame[row].second[col];

        const bool topOn = (ch == L'█') || (ch == L'▀');
        const bool bottomOn = (ch == L'█') || (ch == L'▄');

        if ((py % 2) == 0)
        {
            // Верхняя половинка.
            if (!topOn)
            {
                ch = bottomOn ? L'█' : L'▀';
                cellColor = color; // цвет ячейки — от верхней половинки
            }
        }
        else
        {
            // Нижняя половинка.
            if (!bottomOn)
            {
                ch = topOn ? L'█' : L'▄';
                if (!topOn)
                    cellColor = color; // █ — цвет уже от верха, не трогаем
            }
        }
    }

    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level,
        bool showLevelUp
    )
    {
        int idFigure = nextFigure.GetId();
        int positionFigure = nextFigure.GetPosition();
        int colorFigureAsInt = nextFigure.GetColorAsInt();
        unsigned short shift = 1 << 15;

        for (int y = NEXT_Y1; y < NEXT_Y2 + 1; ++y)
        {
            for (int x = NEXT_X1; x < NEXT_X2 + 1; x += SHIFT_MULTIPLIER, shift >>= 1)
            {
                if (FIGURES[idFigure][positionFigure] & shift)
                {
                    gameField[y].first[x] = L'█';
                    gameField[y].first[x + 1] = L'█';
                    gameField[y].second[x] = colorFigureAsInt;
                    gameField[y].second[x + 1] = colorFigureAsInt;
                }
                else
                {
                    gameField[y].first[x] = L' ';
                    gameField[y].first[x + 1] = L' ';
                }
            }
        }

        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                gameField[y].first[CURRENT_X1 + x] = gameStack[y][x].first ? L'█' : L' ';
                gameField[y].second[CURRENT_X1 + x] = gameStack[y][x].second;
            }
        }

        int currentScore = score;

        for (int i = 0; i < 6; ++i)
        {
            gameField[SCORE_Y].first[SCORE_X - i] = (wchar_t)(currentScore % 10 + L'0');
            currentScore /= 10;
        }

        int currentLevel = level;

        for (int i = 0; i < 2; ++i)
        {
            gameField[LEVEL_Y].first[LEVEL_X - i] = (wchar_t)(currentLevel % 10 + L'0');
            currentLevel /= 10;
        }

        if (currentFigure.GetUse())
        {
            idFigure = currentFigure.GetId();
            positionFigure = currentFigure.GetPosition();
            colorFigureAsInt = currentFigure.GetColorAsInt();
            int currentFigurePositionYStart = currentFigure.GetPositionY();
            int currentFigurePositionYFinish = currentFigure.GetPositionY() + SIDE_SQUARE;
            int currentFigurePositionXStart = currentFigure.GetPositionX();
            int currentFigurePositionXFinish = currentFigure.GetPositionX() + SIDE_SQUARE * SHIFT_MULTIPLIER;
            shift = 1 << 15;

            for (int y = currentFigurePositionYStart; y < currentFigurePositionYFinish; ++y)
            {
                for (int x = currentFigurePositionXStart; x < currentFigurePositionXFinish; x += 2, shift >>= 1)
                {
                    if (FIGURES[idFigure][positionFigure] & shift)
                    {
                        gameField[y].first[x] = L'█';
                        gameField[y].first[x + 1] = L'█';
                        gameField[y].second[x] = colorFigureAsInt;
                        gameField[y].second[x + 1] = colorFigureAsInt;
                    }
                }
            }
        }

        // Вспышка level up: «LEVEL N» по центру поля (оверлей поверх кадра,
        // без паузы). Белый текст: цвет ячейки задаётся second[j] != 0
        // (ViewGameField красит текст цветом ячейки). Показывается в раунде,
        // в котором поднят уровень, и гаснет, когда фигура прошла треть поля
        // (y >= LEVEL_UP_HIDE_Y) — чтобы не прятать падающую фигуру.
        if (showLevelUp && currentFigure.GetUse()
            && currentFigure.GetPositionY() < LEVEL_UP_HIDE_Y)
        {
            const std::wstring banner = L"LEVEL " + std::to_wstring(level);
            const int bannerRow = (CURRENT_Y1 + CURRENT_Y2) / 2;
            const int fieldWidth = CURRENT_X2 - CURRENT_X1 + 1;
            const int bannerX = (fieldWidth - static_cast<int>(banner.size())) / 2;
            for (int i = 0; i < static_cast<int>(banner.size()); ++i)
            {
                const int x = CURRENT_X1 + bannerX + i;
                gameField[bannerRow].first[x] = banner[i];
                gameField[bannerRow].second[x] = static_cast<int>(Colors::WHITE);
            }
        }

        // useTruecolor = true: в Adaptive (Windows Terminal) кадр рисуется
        // truecolor-последовательностями (kColorRgb, классическая палитра
        // Tetris), а не ANSI 16 (палитра терминала). Иначе цвета фигур в игре
        // расходились бы с цветами заставки и с ожидаемой классической палитрой.
        // В Fixed (conhost) truecolor недоступен — используется 16-цветный путь.
        ViewGameField(gameField, true);
    }

    // Анимация стирания очищенных линий: по колонкам (слева направо)
    // стирает ячейки очищенных рядов и выводит промежуточные кадры.
    // Вызывается между PrepareLineClear (пометка + очки, движок) и
    // CommitLineClear (сдвиг стека, движок).
    void AnimateLineClear(
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        int clearRow,
        unsigned int clearMask)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            clearMask & 0x8 ? gameField[clearRow].first[CURRENT_X1 + j] = L' ' : L'i';
            clearMask & 0x4 ? gameField[clearRow + 1].first[CURRENT_X1 + j] = L' ' : L'i';
            clearMask & 0x2 ? gameField[clearRow + 2].first[CURRENT_X1 + j] = L' ' : L'i';
            clearMask & 0x1 ? gameField[clearRow + 3].first[CURRENT_X1 + j] = L' ' : L'i';
            // true: в Adaptive (truecolor) кадры анимации стирания
            // рисуются той же палитрой (kColorRgb), что и обычные
            // кадры. Иначе (useTruecolor=false) они шли бы ANSI 16
            // (палитра терминала) и цвета «прыгали» на момент стирания.
            // В Fixed truecolor недоступен — 16-цветный путь, как и раньше.
            ViewGameField(gameField, true);
        }
    }

    void ViewGameField(
        const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        bool useTruecolor)
    {
        DoubleBufferingBegin();

        // Смещение кадра в буфере: в адаптивном режиме — центрирование
        // (g_console.frameOffset, вычисляется в ComputeCopyRegion),
        // в фиксированном — (0,0).
        const COORD offset = g_console.frameOffset;

        if (g_console.truecolor)
        {
            // Адаптивный режим: весь кадр собирается в одну строку с
            // VT-последовательностями и выводится одним записом в скрытый
            // basicBuffer. Позиционирование строк — CUP (ESC[row;colH],
            // 1-based), т.к. SetConsoleCursorPosition в середине одного
            // записа вызвать нельзя. Атомарность кадра обеспечивает двойная
            // буферизация: предыдущий кадр показывается в doubleBuffer
            // (DoubleBufferingBegin), новый рисуется в basicBuffer, в конце
            // DoubleBufferingEnd переключает активный буфер — терминал
            // мгновенно показывает новый кадр. useTruecolor — truecolor
            // (ESC[38;2;R;G;Bm, kColorRgb), иначе 16-цвет (ANSI 16,
            // ESC[38;5;Nm, палитра терминала).
            auto appendColor = [useTruecolor](std::wstring &s, int c)
            {
                if (useTruecolor)
                {
                    AppendTrueColor(s, c);
                }
                else
                {
                    AppendAnsi16Color(s, c);
                }
            };

            std::wstring frame;
            int row = 0;
            const int bufHeight = static_cast<int>(g_console.csbi.dwSize.Y);
            const int maxCols = static_cast<int>(g_console.csbi.dwSize.X) - offset.X;

            // Начало кадра — LIGHTGRAY, чтобы текст (счёт, уровень) не
            // унаследовал цвет последнего блока предыдущего кадра.
            // lastColor ведётся через весь кадр: цвет терминала не сбрасывается
            // между строками, поэтому сброс делаем точечно — перед видимым
            // символом (не пробелом), если текущий цвет — цвет блока. Иначе
            // текст (счёт, уровень, рамка) унаследует цвет последнего блока.
            appendColor(frame, static_cast<int>(Colors::LIGHTGRAY));
            int lastColor = static_cast<int>(Colors::LIGHTGRAY);

            for (const auto &i : gameField)
            {
                const int bufRow = offset.Y + row;

                // Строки кадра, выходящие за низ буфера (маленькое окно),
                // пропускаем.
                if (bufRow >= bufHeight)
                {
                    ++row;
                    continue;
                }

                std::size_t iFirstSize = i.first.size();
                if (maxCols > 0 && iFirstSize > static_cast<std::size_t>(maxCols))
                {
                    iFirstSize = static_cast<std::size_t>(maxCols);
                }

                AppendCursorPos(frame, bufRow + 1, offset.X + 1);

                // VT-последовательность цвета — только при смене цвета, а не
                // на каждую ячейку (цвета в логотипе идут полосами).
                for (std::size_t j = 0; j < iFirstSize; ++j)
                {
                    if (i.first[j] == L'█')
                    {
                        const int color = i.second[j];
                        if (color != lastColor)
                        {
                            appendColor(frame, color);
                            lastColor = color;
                        }
                        frame += i.first[j];
                    }
                    else if (i.first[j] != L' ')
                    {
                        // Видимый символ (не пробел): цвет ячейки
                        // (second[j] != 0) приоритетен — напр. вспышка
                        // «LEVEL N». Иначе сбрасываем на LIGHTGRAY, чтобы
                        // текст не унаследовал цвет последнего блока.
                        const int textColor = i.second[j] != 0
                            ? i.second[j]
                            : static_cast<int>(Colors::LIGHTGRAY);
                        if (textColor != lastColor)
                        {
                            appendColor(frame, textColor);
                            lastColor = textColor;
                        }
                        frame += i.first[j];
                    }
                    else
                    {
                        frame += i.first[j];
                    }
                }

                ++row;
            }

            std::wcout << frame;
            std::wcout.flush();
            g_console.prevFrame = frame;
            DoubleBufferingEnd();
            return;
        }

        // Фиксированный режим (и фолбэк Adaptive без VT): 16-цвет,
        // посимвольно через SetConsoleTextAttribute + wcout.
        // Доступная ширина строки: в узком буфере строка обрезается,
        // иначе wcout перенесёт хвост на следующую строку и кадр «поедет».
        const int maxCols = static_cast<int>(g_console.csbi.dwSize.X) - offset.X;

        int row = 0;
        const int bufHeight = static_cast<int>(g_console.csbi.dwSize.Y);

        // Курсор в этом пути «течёт»: после кадра он остаётся на последней
        // строке (row 24), и следующий кадр, начатый оттуда, уводит буфер
        // в скролл — кадр сдвигается вниз на строку каждый раз (моргание со
        // смещением по вертикали). Возвращаем курсор в начало кадра: кадр
        // всегда рисуется в строках 0..23, без дрейфа и скролла.
        (void)SetConsoleCursorPosition(g_console.basicBuffer, COORD{ offset.X, offset.Y });

        for (const auto &i : gameField)
        {
            const int bufRow = offset.Y + row;

            // Адаптивный режим: строки кадра, выходящие за низ буфера
            // (маленькое окно), пропускаем — иначе курсор уйдёт за пределы
            // буфера и кадр «поедет».
            if (g_console.mode == ConsoleMode::Adaptive && bufRow >= bufHeight)
            {
                ++row;
                continue;
            }

            if (g_console.mode == ConsoleMode::Adaptive)
            {
                (void)SetConsoleCursorPosition(g_console.basicBuffer, COORD{ offset.X, static_cast<SHORT>(bufRow) });
            }

            std::size_t iFirstSize = i.first.size();

            if (maxCols > 0 && iFirstSize > static_cast<std::size_t>(maxCols))
            {
                iFirstSize = static_cast<std::size_t>(maxCols);
            }

            for (std::size_t j = 0; j < iFirstSize; ++j)
            {
                if (i.first[j] == L'█')
                {
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(i.second[j]));
                    std::wcout << i.first[j];
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(Colors::LIGHTGRAY));
                }
                else if (i.first[j] != L' ' && i.second[j] != 0)
                {
                    // Видимый символ с цветом ячейки (напр. вспышка «LEVEL N»):
                    // рисуем цветом ячейки, затем сбрасываем на LIGHTGRAY.
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(i.second[j]));
                    std::wcout << i.first[j];
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(Colors::LIGHTGRAY));
                }
                else
                {
                    std::wcout << i.first[j];
                }
            }

            std::wcout << std::endl;
            ++row;
        }

        DoubleBufferingEnd();
    }
}
