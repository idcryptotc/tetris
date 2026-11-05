#include "../Core/GameData.h"
#include "../Input/Keys.h"
#include "../Input/Settings.h"
#include "../Rendering/GameField.h"
#include "../Rendering/PixelFont.h"
#include "GameMenu.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include <conio.h>
#include <windows.h>

namespace Tetris
{
    // Подписи пунктов меню (короткие, кириллицей).
    // [0] — главное меню, [1] — меню возврата.
    static const wchar_t *kMenuItems[2][3] =
    {
        { L"НОВАЯ", L"КЛАВИШИ", L"ВЫХОД" },
        { L"НОВАЯ", L"ДАЛЬШЕ", L"ВЫХОД" },
    };

    // Цвет букв заголовка «МЕНЮ» (классическая палитра, как в логотипе).
    static constexpr Colors kTitleColors[4] =
    {
        Colors::CYAN,       // М
        Colors::LIGHTBLUE,  // Е
        Colors::LIGHTGREEN, // Н
        Colors::MAGENTA,    // Ю
    };

    // Раскладка кадра (пиксельные координаты, 2 пикселя на строку символов).
    // Глиф 5x7 пикселей = 4 строки символов.
    static constexpr int kTitleY = 4;  // заголовок (строки 2-5)
    static constexpr int kItem1Y = 16; // пункт 1 (строки 8-11)
    static constexpr int kItem2Y = 26; // пункт 2 (строки 13-16)
    static constexpr int kItem3Y = 36; // пункт 3 (строки 18-21)
    static constexpr int kTitleX = 27; // заголовок центрирован (26 пикселей)
    static constexpr int kItemX = 4;   // пункты слева

    // Рисует строку пиксельным шрифтом в кадре. x, y — пиксельные координаты.
    // Пробел сдвигает курсор, ничего не рисуя. color — цвет всех букв.
    static void DrawPixelString(
        std::vector<std::pair<std::wstring, std::vector<int>>> &frame,
        int x,
        int y,
        const wchar_t *text,
        int color)
    {
        int cx = x;

        for (const wchar_t *p = text; *p; ++p)
        {
            if (*p == L' ')
            {
                cx += kGlyphWidth + 2;
                continue;
            }

            const int idx = GlyphIndex(*p);

            if (idx >= 0)
            {
                const unsigned char (*glyph)[kGlyphWidth] = GlyphAt(idx);

                for (int py = 0; py < kGlyphHeight; ++py)
                {
                    for (int px = 0; px < kGlyphWidth; ++px)
                    {
                        if (glyph[py][px])
                        {
                            SetPixel(frame, cx + px, y + py, color);
                        }
                    }
                }
            }

            cx += kGlyphWidth + 2;
        }
    }

    // Собирает кадр меню (24 строки x 79 колонок, как заставка).
    // isReturnMenu — меню возврата (пункт 2 — «ДАЛЬШЕ»), иначе главное.
    static std::vector<std::pair<std::wstring, std::vector<int>>> BuildMenuFrame(bool isReturnMenu)
    {
        std::vector<std::pair<std::wstring, std::vector<int>>> frame(24);

        for (auto &row : frame)
        {
            row.first.assign(79, L' ');
            row.second.assign(79, 0);
        }

        // Заголовок «МЕНЮ» — по одной букве в своём цвете.
        // В DrawPixelString передаём строку из одной буквы (не &title[i] —
        // это хвост всей строки, и первая буква перекрасила бы все).
        const wchar_t title[] = L"МЕНЮ";

        for (int i = 0; i < 4; ++i)
        {
            const wchar_t letter[2] = { title[i], 0 };
            DrawPixelString(frame, kTitleX + i * (kGlyphWidth + 2), kTitleY, letter, static_cast<int>(kTitleColors[i]));
        }

        // Пункты: цифра — YELLOW, « - ПОДПИСЬ» — LIGHTGRAY.
        const int itemYs[3] = { kItem1Y, kItem2Y, kItem3Y };

        for (int i = 0; i < 3; ++i)
        {
            wchar_t num[2] = { (wchar_t)(L'1' + i), 0 };
            DrawPixelString(frame, kItemX, itemYs[i], num, static_cast<int>(Colors::YELLOW));

            const std::wstring rest = std::wstring(L" - ") + kMenuItems[isReturnMenu ? 1 : 0][i];
            DrawPixelString(frame, kItemX + (kGlyphWidth + 2), itemYs[i], rest.c_str(), static_cast<int>(Colors::LIGHTGRAY));
        }

        return frame;
    }

    // Показывает кадр меню и ждёт выбор цифрой (1-3).
    // isReturnMenu — меню возврата (Esc — то же, что пункт 2).
    static int ShowMenuAndWait(bool isReturnMenu)
    {
        const auto frame = BuildMenuFrame(isReturnMenu);
        ViewGameField(frame, true);

        while (true)
        {
            const int key = _getch();

            if (isReturnMenu && key == static_cast<int>(KeyCodes::ESCAPE))
            {
                // Esc — то же, что пункт 2. Дожидаемся отпускания клавиши,
                // чтобы после возврата в игру IsKeyPressed(EXIT) не сработал сразу.
                while (IsKeyPressed(KeyCodes::ESCAPE))
                {
                    Sleep(10);
                }

                return 2;
            }

            const int menuItem = key - '0';

            if (menuItem >= 1 && menuItem <= 3)
            {
                return menuItem;
            }

            // Неверная клавиша — игнорируем, меню остаётся на экране.
        }
    }

    int StartMenu()
    {
        ClearKeyboardBuffer();
        const int menuItem = ShowMenuAndWait(false);
        system("cls");
        return menuItem;
    }

    MenuResult ReturnToMenu()
    {
        // Игровой цикл заметил нажатие клавиши выхода, но она ещё может быть
        // физически нажата. Дожидаемся отпускания, иначе первое чтение в меню
        // сразу вернёт её (Esc) и меню мгновенно закроется.
        while (IsKeyPressed(KeyFor(BindingAction::EXIT)))
        {
            Sleep(10);
        }

        ClearKeyboardBuffer();
        const int menuItem = ShowMenuAndWait(true);
        system("cls");
        return static_cast<MenuResult>(menuItem);
    }

    void ClearKeyboardBuffer()
    {
        while (_kbhit())
        {
            (void)_getch(); // чистим буфер, символ не нужен
        }
    }
}
