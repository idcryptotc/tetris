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
    // Темп событийного цикла, когда ходов от агента нет (мс): фигура висит
    // в воздухе (гравитации нет), CPU крутить нельзя, при этом ходы из
    // очереди должны применяться быстро.
    constexpr int AGENT_IDLE_SLEEP_MS = 30;
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
    // Гравитации нет: фигура движется только действиями агента
    // (move_left/move_right/rotate/move_down/hard_drop), темп партии равен
    // темпу ходов агента (событийный цикл).
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
                    // Очистка очереди: ходы, накопленные во время game over
                    // (агент «не видел» конец и продолжал ходить), не должны
                    // применяться к новой партии.
                    (void)session.TakeActions();
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
                // Публикуем состояние ДО ожидания restart: агент должен увидеть
                // game_over: true (иначе продолжит ходить, не зная о конце).
                session.SetState(engine.GetState());
                gameActive = false;
                continue;
            }

            // Победа: достигнут MAX_LEVEL. Проверяется после game over,
            // т.к. переполнение стека имеет приоритет (партия проиграна).
            if (step.win)
            {
                ShowWin(engine.GetScore(), engine.GetLevel(), false);
                // Публикуем состояние ДО ожидания restart: агент должен увидеть
                // win: true (иначе продолжит ходить, не зная о победе).
                session.SetState(engine.GetState());
                gameActive = false;
                continue;
            }

            // Фигура ждёт агента: гравитации нет — она движется только
            // move_left/move_right/rotate/move_down/hard_drop. line — сколько
            // рядов фигура опустилась (0..MAX_FALL_LINES): раунд завершается,
            // когда line == MAX_FALL_LINES (фиксация фигуры в стеке).
            int line = 0;
            bool locked = false;

            while (!locked && session.IsRunning())
            {
                // Ходы агента из очереди (тот же путь, что и клавиатура).
                const std::vector<PlayerAction> actions = session.TakeActions();
                bool acted = false;

                for (const PlayerAction action : actions)
                {
                    acted = true;

                    if (action == PlayerAction::Down)
                    {
                        // Мягкий шаг вниз на один ряд. Столкновение с полом/стеком
                        // фиксирует фигуру в стеке (ApplyAction/Down — FallOneStep,
                        // SetFigure); падение на последний ряд
                        // (line == MAX_FALL_LINES) тоже фиксация.
                        if (engine.ApplyAction(PlayerAction::Down, line))
                        {
                            locked = true; // столкновение — зафиксирована в стеке
                            break;
                        }

                        ++line;

                        if (line == MAX_FALL_LINES)
                        {
                            locked = true; // упала на последний ряд — зафиксирована
                            break;
                        }
                    }
                    else if (action == PlayerAction::HardDrop)
                    {
                        // Hard drop — падение до фиксации (как удержание
                        // клавиши в игровом цикле человека).
                        int dropLine = line;
                        bool dropLocked = false;

                        while (dropLine < MAX_FALL_LINES)
                        {
                            if (engine.ApplyAction(PlayerAction::HardDrop, dropLine))
                            {
                                dropLocked = true; // столкновение — зафиксирована
                                break;
                            }

                            ++dropLine;
                        }

                        line = dropLine;

                        // Фиксация: столкновение или падение на последний ряд
                        // (после него line == MAX_FALL_LINES).
                        if (dropLocked || line == MAX_FALL_LINES)
                        {
                            locked = true;
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

                // Публикуем состояние для агента (снимок после кадра):
                // агент видит то же, что и на экране.
                session.SetState(engine.GetState());

                if (!acted)
                {
                    // Ходов от агента нет — не крутим CPU: фигура висит
                    // в воздухе, пока агент не сделает ход.
                    Sleep(AGENT_IDLE_SLEEP_MS);
                }
            }
        }

        channel.Stop();
        channelThread.join();
    }
}
