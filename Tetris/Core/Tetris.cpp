#include "TetrisEngine.h"
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
    // Консольная обёртка над движком: цикл с Sleep, опрос ввода, рендер.
    // Вся игровая логика — в TetrisEngine (чистая, без I/O).
    bool PlayGame()
    {
        TetrisEngine engine;
        engine.Reset();
        std::vector<std::pair<std::wstring, std::vector<int>>> gameField = GAME_FIELD;

        while (true)
        {
            ClearKeyboardBuffer();

            // Очистка линий: пометка + очки (движок), анимация стирания (рендер),
            // сдвиг стека вниз (движок).
            const LineClearInfo clearInfo = engine.PrepareLineClear();
            if (clearInfo.lineCleared)
            {
                AnimateLineClear(gameField, clearInfo.clearRow, clearInfo.clearMask);
                engine.CommitLineClear();
            }

            // Переход к новой фигуре: обновление уровня, game over/победа,
            // текущая = следующая, новая следующая.
            const StepResult step = engine.BeginFigure();

            CreateGameField(
                engine.GetNextFigure(),
                engine.GetCurrentFigure(),
                gameField,
                engine.GetStack(),
                engine.GetScore(),
                engine.GetLevel(),
                engine.GetShowLevelUp());

            if (step.gameOver)
            {
                ShowGameOver(engine.GetScore(), engine.GetLevel());
                return true;
            }

            // Победа: достигнут MAX_LEVEL. Проверяется после game over,
            // т.к. переполнение стека имеет приоритет (партия проиграна).
            if (step.win)
            {
                ShowWin(engine.GetScore(), engine.GetLevel());
                return true;
            }

            for (int line = 0; line < MAX_FALL_LINES; ++line)
            {
                int stepProgress = 0;

                while (stepProgress < COUNT_STEP_CHAPTER)
                {
                    // Если нажато несколько клавиш одновременно, обрабатывается
                    // только первая в этом порядке: LEFT, RIGHT, ROTATE_UP,
                    // ROTATE_DOWN, HARD_DROP, EXIT.
                    if (IsKeyPressed(KeyFor(BindingAction::LEFT)))
                    {
                        engine.ApplyAction(PlayerAction::Left, line);
                        CreateGameField(
                            engine.GetNextFigure(),
                            engine.GetCurrentFigure(),
                            gameField,
                            engine.GetStack(),
                            engine.GetScore(),
                            engine.GetLevel(),
                            engine.GetShowLevelUp());
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::RIGHT)))
                    {
                        engine.ApplyAction(PlayerAction::Right, line);
                        CreateGameField(
                            engine.GetNextFigure(),
                            engine.GetCurrentFigure(),
                            gameField,
                            engine.GetStack(),
                            engine.GetScore(),
                            engine.GetLevel(),
                            engine.GetShowLevelUp());
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::ROTATE_UP)))
                    {
                        engine.ApplyAction(PlayerAction::RotateUp, line);
                        CreateGameField(
                            engine.GetNextFigure(),
                            engine.GetCurrentFigure(),
                            gameField,
                            engine.GetStack(),
                            engine.GetScore(),
                            engine.GetLevel(),
                            engine.GetShowLevelUp());
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::ROTATE_DOWN)))
                    {
                        engine.ApplyAction(PlayerAction::RotateDown, line);
                        CreateGameField(
                            engine.GetNextFigure(),
                            engine.GetCurrentFigure(),
                            gameField,
                            engine.GetStack(),
                            engine.GetScore(),
                            engine.GetLevel(),
                            engine.GetShowLevelUp());
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::HARD_DROP)))
                    {
                        if (line < MAX_FALL_LINES)
                        {
                            if (engine.ApplyAction(PlayerAction::HardDrop, line))
                            {
                                line = MAX_FALL_LINES;
                                break;
                            }

                            ++line;
                            ++stepProgress;
                            CreateGameField(
                                engine.GetNextFigure(),
                                engine.GetCurrentFigure(),
                                gameField,
                                engine.GetStack(),
                                engine.GetScore(),
                                engine.GetLevel(),
                                engine.GetShowLevelUp());
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
                                CreateGameField(
                                    engine.GetNextFigure(),
                                    engine.GetCurrentFigure(),
                                    gameField,
                                    engine.GetStack(),
                                    engine.GetScore(),
                                    engine.GetLevel(),
                                    engine.GetShowLevelUp());
                                break;
                            case MenuResult::Quit:
                                return false;
                        }
                    }

                    // level — 1-индексный (1..25), LEVEL_TIME_STEP —
                    // 0-индексный (0..24): уровень 1 → 500 мс, 25 → 20 мс.
                    Sleep(LEVEL_TIME_STEP[engine.GetLevel() - 1] / COUNT_STEP_CHAPTER);
                    ++stepProgress;
                }

                if (line < MAX_FALL_LINES)
                {
                    if (engine.FallOneStep(line) == FallResult::Locked)
                    {
                        line = MAX_FALL_LINES;
                        break;
                    }
                }

                CreateGameField(
                    engine.GetNextFigure(),
                    engine.GetCurrentFigure(),
                    gameField,
                    engine.GetStack(),
                    engine.GetScore(),
                    engine.GetLevel(),
                    engine.GetShowLevelUp());
            }
        }
    }
}
