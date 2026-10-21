#pragma once

namespace Tetris
{
    // Публичный API приложения: то, что использует main() (см. Main.cpp).
    // Остальные функции Tetris.cpp остаются внутренними.
    // Инициализация консоли — в Console.h (ProgramSettings), меню — в GameMenu.h.

    // Одна партия: true — конец партии (предложить сыграть снова), false — выход в меню/из игры.
    bool PlayGame();
}
