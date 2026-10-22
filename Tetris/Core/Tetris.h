#pragma once

namespace Tetris
{
    // Публичная часть Tetris.cpp: единственная функция, доступная извне (её вызывает main()).
    // Правила игры — в Game.h, рендер — в GameField.h, меню — в GameMenu.h.

    // Одна партия: true — конец партии (предложить сыграть снова), false — выход в меню/из игры.
    bool PlayGame();
}
