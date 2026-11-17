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

    // Пауза (мс) между шагами пакета place: без неё вся расстановка
    // пролетала бы за один кадр и пользователь не видел бы движения фигуры.
    constexpr int AGENT_PLACE_STEP_SLEEP_MS = 30;

    // Сколько рядов помечено к очистке (маска LineClearInfo: биты 3..0,
    // 0x8 — старший ряд).
    static int CountClearedLines(const LineClearInfo &info)
    {
        if (!info.lineCleared)
        {
            return 0;
        }

        const unsigned int lines =
            ((info.clearMask & 0x8) >> 3)
            + ((info.clearMask & 0x4) >> 2)
            + ((info.clearMask & 0x2) >> 1)
            + (info.clearMask & 0x1);

        return static_cast<int>(lines);
    }

    // Инициализация агентского канала: старт фоновой нити (bind/listen/accept),
    // ожидание привязки к порту. Возвращает true при успехе.
    static bool StartAgentChannel(
        AgentChannel &channel,
        AgentGameSession &session,
        std::thread &channelThread)
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
    // (очередь ходов и пакетов place). Состояние движка публикуется в
    // сессию после каждого кадра — агент видит то же, что и на экране.
    //
    // Пакет place (полная расстановка фигуры одной командой) исполняется
    // здесь, в игровом цикле: действия пакета по порядку (кадр и небольшая
    // пауза на каждый шаг), затем — принудительный hard_drop до упора
    // (политика (c)). Завершение пакета (линии очищены, новая фигура
    // выдана) помечается в следующем раунде после фазы очистки — на это
    // ждёт канал (счётчик placeGeneration).
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

        // Пакет place зафиксирован в предыдущем раунде: его завершение
        // (линии очищены, новая фигура выдана, состояние опубликовано)
        // помечается в следующем раунде в фазе очистки после BeginFigure —
        // на это ждёт канал.
        bool placeAwaitingClear = false;

        while (session.IsRunning())
        {
            if (!gameActive)
            {
                // Партия окончена: ждём restart или quit от агента.
                if (session.TakeRestartRequest())
                {
                    // Очистка очереди: ходы/пакеты, накопленные во время
                    // game over (агент «не видел» конец и продолжал ходить),
                    // не должны применяться к новой партии.
                    session.ClearPending();
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

                if (placeAwaitingClear)
                {
                    // Завершаем пакет place: канал соберёт ответ place
                    // (game_over: true, счёт, линии).
                    session.CompletePlace(CountClearedLines(clearInfo), engine.GetScore());
                    placeAwaitingClear = false;
                }

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

                if (placeAwaitingClear)
                {
                    // Завершаем пакет place: канал соберёт ответ place
                    // (win: true, счёт, линии).
                    session.CompletePlace(CountClearedLines(clearInfo), engine.GetScore());
                    placeAwaitingClear = false;
                }

                gameActive = false;
                continue;
            }

            // Публикуем состояние новой фигуры и завершаем предыдущий пакет
            // place (если был): канал собирает ответ place из этого состояния.
            session.SetState(engine.GetState());

            if (placeAwaitingClear)
            {
                session.CompletePlace(CountClearedLines(clearInfo), engine.GetScore());
                placeAwaitingClear = false;
            }

            // Фигура ждёт агента: гравитации нет — она движется только
            // move_left/move_right/rotate/move_down/hard_drop. line — сколько
            // рядов фигура опустилась (0..MAX_FALL_LINES): раунд завершается,
            // когда line == MAX_FALL_LINES (фиксация фигуры в стеке).
            //
            // Пакет place: действия выполняются по порядку (кадр и пауза на
            // каждый шаг — пользователь видит движение фигуры), затем, если
            // фигура ещё не зафиксирована, игра сама опускает её до упора
            // и фиксирует (политика (c)).
            int line = 0;
            bool locked = false;
            bool placeExecuted = false;

            while (!locked
                && session.IsRunning())
            {
                // Ход агента из очереди: одиночный ход (action) или пакет
                // place — тот же путь, что и клавиатура.
                const std::optional<AgentActionBatch> item = session.TakeNextItem();
                bool acted = false;

                if (item.has_value()
                    && !item->actions.empty())
                {
                    if (item->isPlace)
                    {
                        // Старт пакета place: запоминаем счёт (для прироста
                        // в результате).
                        session.MarkPlaceStarted(engine.GetScore());
                        placeExecuted = true;
                    }

                    for (const PlayerAction action : item->actions)
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

                                // Падающий кадр (только для пакета place),
                                // чтобы пользователь видел опускание.
                                if (item->isPlace && dropLine < MAX_FALL_LINES)
                                {
                                    CreateGameField(
                                        engine.GetNextFigure(),
                                        engine.GetCurrentFigure(),
                                        gameField,
                                        engine.GetStack(),
                                        engine.GetScore(),
                                        engine.GetLevel(),
                                        engine.GetShowLevelUp());

                                    Sleep(AGENT_PLACE_STEP_SLEEP_MS);
                                }
                            }

                            line = dropLine;

                            // Фиксация: столкновение или падение на последний ряд
                            // (после него line == MAX_FALL_LINES).
                            if (dropLocked
                                || line == MAX_FALL_LINES)
                            {
                                locked = true;
                                break;
                            }
                        }
                        else
                        {
                            engine.ApplyAction(action, line);
                        }

                        if (locked)
                        {
                            break;
                        }

                        CreateGameField(
                            engine.GetNextFigure(),
                            engine.GetCurrentFigure(),
                            gameField,
                            engine.GetStack(),
                            engine.GetScore(),
                            engine.GetLevel(),
                            engine.GetShowLevelUp());

                        // Пауза после шага пакета, чтобы пользователь видел
                        // движение фигуры: без неё вся расстановка пролетала
                        // бы за один кадр.
                        if (item->isPlace)
                        {
                            Sleep(AGENT_PLACE_STEP_SLEEP_MS);
                        }
                    }

                    // Политика (c): агент описал полную расстановку — игра
                    // сама опускает фигуру до упора и фиксирует (пошагово,
                    // с кадрами).
                    if (item->isPlace
                        && !locked)
                    {
                        while (!locked
                            && line < MAX_FALL_LINES)
                        {
                            if (engine.ApplyAction(PlayerAction::HardDrop, line))
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

                            CreateGameField(
                                engine.GetNextFigure(),
                                engine.GetCurrentFigure(),
                                gameField,
                                engine.GetStack(),
                                engine.GetScore(),
                                engine.GetLevel(),
                                engine.GetShowLevelUp());

                            Sleep(AGENT_PLACE_STEP_SLEEP_MS);
                        }
                    }
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

            // Пакет place зафиксирован в этом раунде: его завершение (линии
            // очищены, новая фигура выдана) выполнится в следующем раунде,
            // в фазе очистки после BeginFigure.
            placeAwaitingClear = placeExecuted;
        }

        channel.Stop();
        channelThread.join();
    }
}
