#include "App.h"
#include "Console.h"
#include "Game.h"
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
    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level
    );

    void ViewGameField(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField);
    int PositionById(int figureId);
    MenuResult ReturnToMenu();
    int RandomInt(int bound);
    int RandomFigureId();
    void ClearKeyboardBuffer();

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
