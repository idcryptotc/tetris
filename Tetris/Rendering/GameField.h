#pragma once

#include "../Core/Figure.h"

#include <string>
#include <utility>
#include <vector>

namespace Tetris
{
    // Рендер игрового поля: сборка кадра и вывод его в консоль.

    // Собирает кадр (gameField) из стека, следующей и текущей фигур,
    // счёта и уровня, затем выводит его.
    // showLevelUp — показать вспышку «LEVEL N» (повышение уровня: белый
    // текст по центру поля, оверлей поверх кадра, без паузы).
    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level,
        bool showLevelUp = false
    );

    // Выводит кадр в консоль (с двойной буферизацией).
    // useTruecolor — рисовать цветные ячейки (█) через truecolor (VT,
    // только Adaptive); по умолчанию false — 16-цветный путь.
    void ViewGameField(
        const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        bool useTruecolor = false);
}
