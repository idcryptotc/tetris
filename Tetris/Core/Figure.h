#pragma once

#include "GameData.h"

namespace Tetris
{
    class Figure
    {
    private:
        int id{};
        Colors color{ Colors::WHITE };
        int position{};
        int nextPosition{ position };
        int x{ BASIC_X };
        int y{ BASIC_Y };
        int nextX{ x };
        int nextY{ y + 1 };
        int multiplier{ SHIFT_MULTIPLIER };
        bool isUse{ true };

    public:
        Figure() = default;

        Figure(int id, int p);

        void SetBasicParameters(int figureId, int figurePosition);

        void SetPosition(int figurePosition);

        void ShiftRight();

        void ShiftLeft();

        void ShiftDown();

        void RotateUp();

        void RotateDown();

        int GetPositionX(bool isNext = false) const;

        int GetPositionY(bool isNext = false) const;

        int GetId() const;

        int GetColorAsInt() const;

        int GetPosition(bool isNext = false) const;

        void Action(Actions action);

        void SetUse(bool state);

        bool GetUse() const;
    };
}
