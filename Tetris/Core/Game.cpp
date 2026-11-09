#include "Figure.h"
#include "Game.h"
#include "GameData.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Tetris
{

    // Проверяет, не пересекается ли «пробная» позиция фигуры со стеком.
    // Стек не копируется и не изменяется: тетрамино не пересекает само себя,
    // поэтому достаточно проверить, занята ли уже любая из его ячеек.
    bool CheckCollisions(const Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction)
    {
        bool isXNext = direction == MoveDirection::Left
            || direction == MoveDirection::Right;

        bool isYNext = direction == MoveDirection::Drop
            && currentFigure.GetPositionY(true) != static_cast<int>(gameStack.size()) - 3;

        bool isPositionNext = direction != MoveDirection::Drop;
        int currentFigurePositionYStart = currentFigure.GetPositionY(isYNext);
        int currentFigurePositionYFinish = currentFigure.GetPositionY(isYNext) + SIDE_SQUARE;
        int currentFigurePositionXStart = currentFigure.GetPositionX(isXNext) - CURRENT_X1;
        int currentFigurePositionXFinish = currentFigure.GetPositionX(isXNext) - CURRENT_X1 + SIDE_SQUARE * SHIFT_MULTIPLIER;
        int currentFigureId = currentFigure.GetId();
        int currentFigurePosition = currentFigure.GetPosition(isPositionNext);
        unsigned short shift = 1 << 15;

        for (int y = currentFigurePositionYStart; y < currentFigurePositionYFinish; ++y)
        {
            for (int x = currentFigurePositionXStart;
                x < currentFigurePositionXFinish;
                x += 2, shift >>= 1)
            {
                if (FIGURES[currentFigureId][currentFigurePosition] & shift)
                {
                    if (gameStack[y][x].first >= 1 || gameStack[y][x + 1].first >= 1)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    // Делает «пробное» движение фигуры, проверяет столкновение
    // и либо подтверждает его (commitAction), либо откатывает (DEFAULT).
    bool TryMove(Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction, Actions commitAction)
    {
        switch (direction)
        {
            case MoveDirection::Left:
                currentFigure.ShiftLeft();
                break;
            case MoveDirection::Right:
                currentFigure.ShiftRight();
                break;
            case MoveDirection::RotateUp:
                currentFigure.RotateUp();
                break;
            case MoveDirection::RotateDown:
                currentFigure.RotateDown();
                break;
            case MoveDirection::Drop:
                currentFigure.ShiftDown();
                break;
        }

        if (CheckCollisions(currentFigure, gameStack, direction))
        {
            currentFigure.Action(Actions::DEFAULT); // откат пробного движения
            return false;
        }

        currentFigure.Action(commitAction);
        return true;
    }

    // Сдвигает фигуру на ряд вниз. Locked — фигура зафиксирована в стеке.
    FallResult TryFallOneStep(Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack, int line)
    {
        if (line >= MAX_FALL_LINES)
        {
            return FallResult::Locked;
        }

        currentFigure.ShiftDown();

        if (CheckCollisions(currentFigure, gameStack, MoveDirection::Drop))
        {
            SetFigure(currentFigure, gameStack);
            return FallResult::Locked;
        }

        currentFigure.Action(Actions::DOWN);

        if (line == LAST_FALL_LINE)
        {
            SetFigure(currentFigure, gameStack);
            currentFigure.SetUse(false);
        }

        return FallResult::Fell;
    }

    void SetFigure(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack)
    {
        int lastLine = (currentFigure.GetPositionY() + 3) == gameStack.size() ? 1 : 0;
        int positionYStart = currentFigure.GetPositionY() - lastLine;
        int positionYFinish = currentFigure.GetPositionY() + SIDE_SQUARE - lastLine;
        int positionXStart = currentFigure.GetPositionX() - CURRENT_X1;
        int positionXFinish = currentFigure.GetPositionX() - CURRENT_X1 + SIDE_SQUARE * SHIFT_MULTIPLIER;
        int currentFigureId = currentFigure.GetId();
        int currentFigurePosition = currentFigure.GetPosition();
        int currentFigureColorAsInt = currentFigure.GetColorAsInt();
        unsigned short shift = 1 << 15;

        for (int y = positionYStart; y < positionYFinish; ++y)
        {
            for (int x = positionXStart; x < positionXFinish; x += 2, shift >>= 1)
            {
                if (FIGURES[currentFigureId][currentFigurePosition] & shift)
                {
                    ++gameStack[y][x].first;
                    ++gameStack[y][x + 1].first;
                    gameStack[y][x].second = currentFigureColorAsInt;
                    gameStack[y][x + 1].second = currentFigureColorAsInt;
                    ++gameStack[y][WIDTH].first;
                }
            }
        }
    }

    bool CheckGameOver(const std::vector<std::vector<std::pair<int, int>>> &gameStack)
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            for (const auto &j : gameStack[i])
            {
                if (j.first > 0)
                {
                    return true;
                }
            }
        }

        return false;
    }


    void CheckScoreForLevel(const unsigned int &score, int &level)
    {
        std::size_t size = LEVEL_SCORE.size();

        for (std::size_t i = level - 1; i < size; ++i)
        {
            if (score > LEVEL_SCORE[i])
            {
                level = static_cast<int>(i) + 1;
            }
            else
            {
                break;
            }
        }
    }
}
