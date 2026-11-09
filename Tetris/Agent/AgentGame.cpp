#include "AgentGame.h"

#include "AgentChannel.h"
#include "AgentGameSession.h"
#include "../Core/TetrisEngine.h"
#include "../Core/GameData.h"
#include "../Rendering/GameEnd.h"
#include "../Rendering/GameField.h"
#include "../Menu/GameMenu.h"

#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <windows.h>

namespace Tetris
{
    // Время на фигуру в агентском режиме (мс). Фиксированное, не зависит
    // от уровня: темп игры для человека (LEVEL_TIME_STEP, 500→20 мс) агенту
    // не подходит — MCP-раундтрип (запрос состояния + ход) занимает секунды.
    // 40 с на фигуру — запас на «раздумья» агента.
    constexpr int AGENT_FIGURE_TIME_MS = 40000;
    // Инициализация агентского канала: старт фоновой нити (bind/listen/accept),
    // ожидание привязки к порту. Возвращает true при успехе.
    static bool StartAgentChannel(AgentChannel &channel, AgentGameSession &session, std::thread &channelThread)
    {
        channel.SetSession(&session);
        channelThread = std::thread([&channel]
        {
            (void)channel.Start();
        });

        // Ждём, пока фоновая нить привяжется к порту (до 5 с).
        for (int i = 0; i < 100; ++i)
        {
            if (channel.GetBoundPort() != -1)
            {
                return true;
            }

            Sleep(50);
        }

        return false;
    }

    // Агентский режим: игра без ввода с клавиатуры. Ходы приходят из
    // агентского канала (AgentChannel, фоновый поток) через AgentGameSession
    // (очередь действий). Состояние движка публикуется в сессию после каждого
    // кадра — агент видит то же, что и на экране.
    //
    // Возвращается, когда агент запрашивает выход (команда quit).
    void PlayAgentGame()
    {
        TetrisEngine engine;
        engine.Reset();
        std::vector<std::pair<std::wstring, std::vector<int>>> gameField = GAME_FIELD;

        AgentGameSession session;
        AgentChannel channel;
        std::thread channelThread;

        if (!StartAgentChannel(channel, session, channelThread))
        {
            channel.Stop();
            channelThread.join();
            std::wcout << L"Агентский канал: все порты 7777-7786 заняты.\n";
            return;
        }

        bool gameActive = true;

        while (session.IsRunning())
        {
            if (!gameActive)
            {
                // Партия окончена: ждём restart или quit от агента.
                if (session.TakeRestartRequest())
                {
                    engine.Reset();
                    gameField = GAME_FIELD;
                    gameActive = true;
                }
                else
                {
                    Sleep(50);
                }

                continue;
            }

            ClearKeyboardBuffer();

            // Очистка линий: пометка + очки (движок), анимация стирания
            // (рендер), сдвиг стека вниз (движок).
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
                ShowGameOver(engine.GetScore(), engine.GetLevel(), false);
                gameActive = false;
                continue;
            }

            // Победа: достигнут MAX_LEVEL. Проверяется после game over,
            // т.к. переполнение стека имеет приоритет (партия проиграна).
            if (step.win)
            {
                ShowWin(engine.GetScore(), engine.GetLevel(), false);
                gameActive = false;
                continue;
            }

            for (int line = 0; line < MAX_FALL_LINES; ++line)
            {
                int stepProgress = 0;
                bool roundOver = false;

                while (stepProgress < COUNT_STEP_CHAPTER && !roundOver)
                {
                    // Ходы агента из очереди (тот же путь, что и клавиатура).
                    const std::vector<PlayerAction> actions = session.TakeActions();
                    for (const PlayerAction action : actions)
                    {
                        if (action == PlayerAction::HardDrop)
                        {
                            // Hard drop — падение до фиксации (как удержание
                            // клавиши в игровом цикле человека).
                            int dropLine = line;
                            bool locked = false;

                            while (dropLine < MAX_FALL_LINES)
                            {
                                if (engine.ApplyAction(PlayerAction::HardDrop, dropLine))
                                {
                                    locked = true;
                                    break;
                                }

                                ++dropLine;
                            }

                            line = dropLine;

                            if (locked)
                            {
                                // Фигура зафиксирована в стеке — конец раунда
                                // (как в игровом цикле человека: line = MAX_FALL_LINES,
                                // break из цикла шагов).
                                line = MAX_FALL_LINES;
                                roundOver = true;
                                break;
                            }
                        }
                        else
                        {
                            engine.ApplyAction(action, line);
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

                    // Фиксированный темп для агента: AGENT_FIGURE_TIME_MS на
                    // фигуру (19 рядов × 8 шагов = 152 шага), независимо от
                    // уровня. Набрав скорости (LEVEL_TIME_STEP) агент не
                    // успевает — MCP-раундтрип занимает секунды.
                    Sleep(AGENT_FIGURE_TIME_MS / (MAX_FALL_LINES * COUNT_STEP_CHAPTER));
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

                // Публикуем состояние для агента (снимок после кадра):
                // агент видит то же, что и на экране, после каждого шага.
                session.SetState(engine.GetState());
            }
        }

        channel.Stop();
        channelThread.join();
    }
}
