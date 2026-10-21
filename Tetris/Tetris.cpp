#include "App.h"
#include "Game.h"
#include "GameData.h"
#include "GameField.h"
#include "GameMenu.h"
#include "Figure.h"
#include "FigureGenerator.h"
#include "Keys.h"
#include "Settings.h"

#include <string>
#include <utility>
#include <vector>

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
            CheckScoreForLevel(score, levelGame);
            currentFigure = nextFigure;
            currentFigure.SetUse(true);
            figureId = RandomFigureId();
            figurePosition = PositionById(figureId);
            nextFigure.SetBasicParameters(figureId, figurePosition);
            CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);

            if (CheckGameOver(gameStack))
            {
                return true;
            }

            for (int line = 0; line < MAX_FALL_LINES; ++line)
            {
                stepProgress = 0;

                while (stepProgress < COUNT_STEP_CHAPTER)
                {
                    // Приоритет клавиш сохранён как в исходной версии:
                    // LEFT > RIGHT > ROTATE_UP > ROTATE_DOWN > HARD_DROP > EXIT.
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
