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
    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level
    );

    // Выводит кадр в консоль (с двойной буферизацией).
    void ViewGameField(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField);

    // Выводит кадр в консоль (с двойной буферизацией). Буферы инвертированы.
    void ViewGameFieldInvert(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField);
}
