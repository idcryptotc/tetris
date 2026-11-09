#pragma once

#include "../Core/TetrisEngine.h"

#include <deque>
#include <mutex>
#include <vector>

namespace Tetris
{
    // Общее состояние между игровым циклом (основной поток) и агентским
    // каналом (фоновый поток). Движок (TetrisEngine) живёт в игровом цикле
    // и изменяется только им; этот класс хранит лишь данные, которыми
    // обмениваются потоки (очередь действий, снимок состояния, флаги),
    // защищённые мьютексом.
    class AgentGameSession
    {
    private:
        mutable std::mutex mutex;
        std::deque<PlayerAction> pendingActions;
        GameStateSnapshot currentState;
        bool running{ true };
        bool restartRequested{ false };

    public:
        // Агент: добавить действие в очередь (игровой цикл применит его).
        void EnqueueAction(PlayerAction action);

        // Игровой цикл: забрать все накопленные действия и очистить очередь.
        std::vector<PlayerAction> TakeActions();

        // Игровой цикл: обновить снимок состояния (после каждого шага).
        void SetState(const GameStateSnapshot &state);

        // Агент: получить текущий снимок состояния.
        GameStateSnapshot GetState() const;

        // Агент: запросить новую партию (после game over/победы).
        void RequestRestart();

        // Игровой цикл: забрать запрос рестарта (true и сброс флага).
        bool TakeRestartRequest();

        // Агент: завершить игру.
        void RequestQuit();

        // Игровой цикл: идёт ли игра.
        bool IsRunning() const;
    };
}
