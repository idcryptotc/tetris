#include "AgentGameSession.h"

#include <utility>

namespace Tetris
{
    // Агент: добавить действие в очередь (игровой цикл применит его).
    void AgentGameSession::EnqueueAction(PlayerAction action)
    {
        std::lock_guard<std::mutex> lock(mutex);
        pendingActions.push_back(action);
    }

    // Игровой цикл: забрать все накопленные действия и очистить очередь.
    std::vector<PlayerAction> AgentGameSession::TakeActions()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<PlayerAction> actions;
        actions.reserve(pendingActions.size());
        for (const PlayerAction &action : pendingActions)
        {
            actions.push_back(action);
        }
        pendingActions.clear();
        return actions;
    }

    // Игровой цикл: обновить снимок состояния (после каждого шага).
    void AgentGameSession::SetState(const GameStateSnapshot &state)
    {
        std::lock_guard<std::mutex> lock(mutex);
        currentState = state;
    }

    // Агент: получить текущий снимок состояния.
    GameStateSnapshot AgentGameSession::GetState() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return currentState;
    }

    // Агент: запросить новую партию (после game over/победы).
    void AgentGameSession::RequestRestart()
    {
        std::lock_guard<std::mutex> lock(mutex);
        restartRequested = true;
    }

    // Игровой цикл: забрать запрос рестарта (true и сброс флага).
    bool AgentGameSession::TakeRestartRequest()
    {
        std::lock_guard<std::mutex> lock(mutex);
        bool requested = restartRequested;
        restartRequested = false;
        return requested;
    }

    // Агент: завершить игру.
    void AgentGameSession::RequestQuit()
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = false;
    }

    // Игровой цикл: идёт ли игра.
    bool AgentGameSession::IsRunning() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return running;
    }
}
