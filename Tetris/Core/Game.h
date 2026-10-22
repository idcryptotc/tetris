#pragma once

#include "Figure.h"
#include "GameData.h"

#include <string>
#include <utility>
#include <vector>

namespace Tetris
{
    // Правила игры: столкновения, движения, падение, фиксация,
    // очистка линий, подсчёт очков и уровней.

    // Проверяет, не пересекается ли «пробная» позиция фигуры со стеком.
    // Стек не копируется и не изменяется: тетрамино не пересекает само себя,
    // поэтому достаточно проверить, занята ли уже любая из его ячеек.
    bool CheckCollisions(const Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction);

    // Делает «пробное» движение фигуры, проверяет столкновение
    // и либо подтверждает его (commitAction), либо откатывает (DEFAULT).
    bool TryMove(Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction, Actions commitAction);

    // Сдвигает фигуру на ряд вниз. Locked — фигура зафиксирована в стеке.
    FallResult TryFallOneStep(Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack, int line);

    // Фиксирует фигуру в стеке (при падении или столкновении).
    void SetFigure(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack);

    // Проверяет, не заполнены ли верхние ряды стека (конец партии).
    bool CheckGameOver(const std::vector<std::vector<std::pair<int, int>>> &gameStack);

    // Очищает заполненные строки стека, начисляет очки и сдвигает поле вниз.
    void ClearLines
    (
        std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const Figure &nextFigure,
        unsigned int &score,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const int &level
    );

    // Повышает уровень, если счёт превысил порог.
    void CheckScoreForLevel(const unsigned int &score, int &level);
}
