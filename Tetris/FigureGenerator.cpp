#include "FigureGenerator.h"
#include "GameData.h"

#include <random>

namespace Tetris
{
    // Случайное целое в диапазоне [0, bound).
    int RandomInt(int bound)
    {
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution<int> dist(0, bound - 1);
        return dist(rng);
    }

    int RandomFigureId()
    {
        return RandomInt(static_cast<int>(FIGURES.size()));
    }

    int PositionById(int figureId)
    {
        switch (figureId)
        {
            case 0:case 1:case 2:
                {
                    return RandomInt(10);
                }
            case 3:case 4:case 5:
                {
                    return RandomInt(5);
                }
            case 6:
                {
                    return RandomInt(3);
                }
            default:
                {
                    return -1;
                }
        }
    }
}
