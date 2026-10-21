#include "App.h"
#include "Game.h"
#include "GameField.h"
#include "GameData.h"
#include "Figure.h"
#include "Keys.h"
#include "Settings.h"

#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <conio.h>

namespace Tetris
{
    int PositionById(int figureId);
    MenuResult ReturnToMenu();
    int RandomInt(int bound);
    int RandomFigureId();
    void ClearKeyboardBuffer();



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

    int StartMenu()
    {
        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Настройка клавиш" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;
            menuItem = _getch() - '0';

            if (menuItem < 1 || menuItem > 3)
            {
                system("cls");
                continue;
            }

            system("cls");
            return menuItem;
        }
    }

    MenuResult ReturnToMenu()
    {
        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Продолжить" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;
            menuItem = _getch() - '0';

            if (menuItem < 1 || menuItem > 3)
            {
                system("cls");
            }
            else
            {
                break;
            }
        }

        return static_cast<MenuResult>(menuItem);
    }

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

    void ClearKeyboardBuffer()
    {
        while (_kbhit())
        {
            (void)_getch(); // чистим буфер, символ не нужен
        }
    }
}
