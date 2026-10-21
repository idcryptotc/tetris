#include "Figure.h"

namespace Tetris
{
    Figure::Figure(int id, int p) : id(id), position(p)
    {
        color = FIGURE_COLORS.at(id);
    }

    void Figure::SetBasicParameters(int figureId, int figurePosition)
    {
        id = figureId;
        color = FIGURE_COLORS.at(id);
        position = figurePosition;
        nextPosition = position;
        x = BASIC_X;
        y = BASIC_Y;
        nextX = x;
        nextY = y + 1;
        isUse = true;
    }

    void Figure::SetPosition(int figurePosition)
    {
        position = figurePosition;
    }

    void Figure::ShiftRight()
    {
        if (x < CURRENT_X2 - 7)
        {
            nextX = x + multiplier;
        }
        else
        {
            if (NEXT_POSITION_RIGHT[id].find(position) != NEXT_POSITION_RIGHT[id].end())
            {
                nextPosition = NEXT_POSITION_RIGHT[id].at(position);
            }
        }
    }

    void Figure::ShiftLeft()
    {
        if (x > CURRENT_X1)
        {
            nextX = x - multiplier;
        }
        else
        {
            if (NEXT_POSITION_LEFT[id].find(position) != NEXT_POSITION_LEFT[id].end())
            {
                nextPosition = NEXT_POSITION_LEFT[id].at(position);
            }
        }
    }

    void Figure::ShiftDown()
    {
        nextY = y + 1;
    }

    void Figure::RotateUp()
    {
        if (NEXT_POSITION_ROTATE_UP[id].find(position) != NEXT_POSITION_ROTATE_UP[id].end())
        {
            nextPosition = NEXT_POSITION_ROTATE_UP[id].at(position);
        }
    }

    void Figure::RotateDown()
    {
        if (NEXT_POSITION_ROTATE_DOWN[id].find(position) != NEXT_POSITION_ROTATE_DOWN[id].end())
        {
            nextPosition = NEXT_POSITION_ROTATE_DOWN[id].at(position);
        }
    }

    int Figure::GetPositionX(bool isNext) const
    {
        return isNext
            ? nextX
            : x;
    }

    int Figure::GetPositionY(bool isNext) const
    {
        return isNext
            ? nextY
            : y;
    }

    int Figure::GetId() const
    {
        return id;
    }

    int Figure::GetColorAsInt() const
    {
        return (int)color;
    }

    int Figure::GetPosition(bool isNext) const
    {
        return isNext
            ? nextPosition
            : position;
    }

    void Figure::Action(Actions action)
    {
        switch (action)
        {
            case Actions::LEFT:
                x = nextX;
                position = nextPosition;
                break;
            case Actions::RIGHT:
                x = nextX;
                position = nextPosition;
                break;
            case Actions::DOWN:
                y = nextY > 18 ? 18 : nextY;
                break;
            case Actions::ROTATE:
                position = nextPosition;
                break;
            case Actions::DEFAULT:
                break;
        }

        nextX = x;
        nextY = (y + 1) > 18 ? 18 : y + 1;
    }

    void Figure::SetUse(bool state)
    {
        isUse = state;
    }

    bool Figure::GetUse() const
    {
        return isUse;
    }
}
