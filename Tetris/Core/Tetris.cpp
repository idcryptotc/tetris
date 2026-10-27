#include "Figure.h"
#include "FigureGenerator.h"
#include "Game.h"
#include "GameData.h"
#include "../Rendering/Console.h"
#include "../Rendering/GameEnd.h"
#include "../Rendering/GameField.h"
#include "../Menu/GameMenu.h"
#include "../Input/Keys.h"
#include "../Input/Settings.h"

#include <string>
#include <utility>
#include <vector>

#include <windows.h>

namespace Tetris
{
    bool PlayGame()
    {
        int levelGame = 1;
        unsigned int score = 0;
        int stepProgress;
        int figureId = RandomFigureId();
        int figurePosition = PositionById(figureId);
        Figure nextFigure(figureId, figurePosition);
        figureId = RandomFigureId();
        figurePosition = PositionById(figureId);
        Figure currentFigure;
        std::vector<std::vector<std::pair<int, int>>> gameStack(HEIGHT, std::vector<std::pair<int, int>>(WIDTH + 1, { 0,0 }));
        std::vector<std::pair<std::wstring, std::vector<int>>> gameField = GAME_FIELD;

        while (true)
        {
            ClearKeyboardBuffer();
            ClearLines(gameStack, nextFigure, score, gameField, levelGame);
            // Уровень ограничен MAX_LEVEL: иначе LEVEL_TIME_STEP[levelGame]
            // (25 элементов, индексы 0..24) при levelGame = 25 — выход за
            // границы (OOB). Победа проверяется ниже.
            CheckScoreForLevel(score, levelGame);
            if (levelGame > MAX_LEVEL)
            {
                levelGame = MAX_LEVEL;
            }
            currentFigure = nextFigure;
            currentFigure.SetUse(true);
            figureId = RandomFigureId();
            figurePosition = PositionById(figureId);
            nextFigure.SetBasicParameters(figureId, figurePosition);
            CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);

            if (CheckGameOver(gameStack))
            {
                ShowGameOver(score, levelGame);
                return true;
            }

            // Победа: достигнут MAX_LEVEL. Проверяется после game over,
            // т.к. переполнение стека имеет приоритет (партия проиграна).
            if (levelGame >= MAX_LEVEL)
            {
                ShowWin(score, levelGame);
                return true;
            }

            for (int line = 0; line < MAX_FALL_LINES; ++line)
            {
                stepProgress = 0;

                while (stepProgress < COUNT_STEP_CHAPTER)
                {
                    // Если нажато несколько клавиш одновременно, обрабатывается
                    // только первая в этом порядке: LEFT, RIGHT, ROTATE_UP,
                    // ROTATE_DOWN, HARD_DROP, EXIT.
                    if (IsKeyPressed(KeyFor(BindingAction::LEFT)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::Left, Actions::LEFT);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::RIGHT)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::Right, Actions::RIGHT);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::ROTATE_UP)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::RotateUp, Actions::ROTATE);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::ROTATE_DOWN)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::RotateDown, Actions::ROTATE);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::HARD_DROP)))
                    {
                        if (line < MAX_FALL_LINES)
                        {
                            if (TryFallOneStep(currentFigure, gameStack, line) == FallResult::Locked)
                            {
                                line = MAX_FALL_LINES;
                                break;
                            }

                            ++line;
                            ++stepProgress;
                            CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                        }
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::EXIT)))
                    {
                        switch (ReturnToMenu())
                        {
                            case MenuResult::Restart:
                                return true;
                            case MenuResult::Stay:
                                // В адаптивном режиме кадр центрируется и не
                                // покрывает весь буфер — текст меню паузы в
                                // левом верхнем углу остался бы видимым.
                                ClearConsoleBuffer();
                                CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                                break;
                            case MenuResult::Quit:
                                return false;
                        }
                    }

                    Sleep(LEVEL_TIME_STEP[levelGame] / COUNT_STEP_CHAPTER);
                    ++stepProgress;
                }

                if (line < MAX_FALL_LINES)
                {
                    if (TryFallOneStep(currentFigure, gameStack, line) == FallResult::Locked)
                    {
                        line = MAX_FALL_LINES;
                        break;
                    }
                }

                CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
            }

            nextFigure.SetUse(false);
        }
    }
}
