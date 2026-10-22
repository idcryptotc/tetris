#include "Console.h"
#include "../Core/Figure.h"
#include "../Core/GameData.h"
#include "GameField.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>

namespace Tetris
{
    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level
    )
    {
        int idFigure = nextFigure.GetId();
        int positionFigure = nextFigure.GetPosition();
        int colorFigureAsInt = nextFigure.GetColorAsInt();
        unsigned short shift = 1 << 15;

        for (int y = NEXT_Y1; y < NEXT_Y2 + 1; ++y)
        {
            for (int x = NEXT_X1; x < NEXT_X2 + 1; x += SHIFT_MULTIPLIER, shift >>= 1)
            {
                if (FIGURES[idFigure][positionFigure] & shift)
                {
                    gameField[y].first[x] = L'█';
                    gameField[y].first[x + 1] = L'█';
                    gameField[y].second[x] = colorFigureAsInt;
                    gameField[y].second[x + 1] = colorFigureAsInt;
                }
                else
                {
                    gameField[y].first[x] = L' ';
                    gameField[y].first[x + 1] = L' ';
                }
            }
        }

        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                gameField[y].first[CURRENT_X1 + x] = gameStack[y][x].first ? L'█' : L' ';
                gameField[y].second[CURRENT_X1 + x] = gameStack[y][x].second;
            }
        }

        int currentScore = score;

        for (int i = 0; i < 6; ++i)
        {
            gameField[SCORE_Y].first[SCORE_X - i] = (wchar_t)(currentScore % 10 + L'0');
            currentScore /= 10;
        }

        int currentLevel = level;

        for (int i = 0; i < 2; ++i)
        {
            gameField[LEVEL_Y].first[LEVEL_X - i] = (wchar_t)(currentLevel % 10 + L'0');
            currentLevel /= 10;
        }

        if (currentFigure.GetUse())
        {
            idFigure = currentFigure.GetId();
            positionFigure = currentFigure.GetPosition();
            colorFigureAsInt = currentFigure.GetColorAsInt();
            int currentFigurePositionYStart = currentFigure.GetPositionY();
            int currentFigurePositionYFinish = currentFigure.GetPositionY() + SIDE_SQUARE;
            int currentFigurePositionXStart = currentFigure.GetPositionX();
            int currentFigurePositionXFinish = currentFigure.GetPositionX() + SIDE_SQUARE * SHIFT_MULTIPLIER;
            shift = 1 << 15;

            for (int y = currentFigurePositionYStart; y < currentFigurePositionYFinish; ++y)
            {
                for (int x = currentFigurePositionXStart; x < currentFigurePositionXFinish; x += 2, shift >>= 1)
                {
                    if (FIGURES[idFigure][positionFigure] & shift)
                    {
                        gameField[y].first[x] = L'█';
                        gameField[y].first[x + 1] = L'█';
                        gameField[y].second[x] = colorFigureAsInt;
                        gameField[y].second[x + 1] = colorFigureAsInt;
                    }
                }
            }
        }

        ViewGameField(gameField);
    }

    void ViewGameField(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField)
    {
        DoubleBufferingBegin();

        for (const auto &i : gameField)
        {
            std::size_t iFirstSize = i.first.size();

            for (std::size_t j = 0; j < iFirstSize; ++j)
            {
                if (i.first[j] == L'█')
                {
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(i.second[j]));
                    std::wcout << i.first[j];
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(Colors::LIGHTGRAY));
                }
                else
                {
                    std::wcout << i.first[j];
                }
            }

            std::wcout << std::endl;
        }

        DoubleBufferingEnd();
    }

    void ViewGameFieldInvert(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField)
    {
        DoubleBufferingEnd();

        for (const auto &i : gameField)
        {
            std::size_t iFirstSize = i.first.size();

            for (std::size_t j = 0; j < iFirstSize; ++j)
            {
                if (i.first[j] == L'█')
                {
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(i.second[j]));
                    std::wcout << i.first[j];
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(Colors::LIGHTGRAY));
                }
                else
                {
                    std::wcout << i.first[j];
                }
            }

            std::wcout << std::endl;
        }

        DoubleBufferingBegin();
    }
    }
