#include "AgentGameSession.h"

#include <utility>
#include <vector>

namespace Tetris
{
    // Агент: добавить действие в очередь (игровой цикл применит его).
    void AgentGameSession::EnqueueAction(PlayerAction action)
    {
        AgentActionBatch batch;
        batch.actions.push_back(action);
        batch.isPlace = false;

        std::lock_guard<std::mutex> lock(mutex);
        pendingItems.push_back(std::move(batch));
    }

    // Агент: добавить пакет place в очередь (полная расстановка фигуры).
    void AgentGameSession::EnqueuePlace(std::vector<PlayerAction> actions)
    {
        AgentActionBatch batch;
        batch.actions = std::move(actions);
        batch.isPlace = true;

        std::lock_guard<std::mutex> lock(mutex);
        pendingItems.push_back(std::move(batch));
    }

    // Игровой цикл: забрать следующий элемент очереди (при отсутствии —
    // nullopt), очистив очередь.
    std::optional<AgentActionBatch> AgentGameSession::TakeNextItem()
    {
        return TakeItem();
    }

    // Игровой цикл: очистить очередь (пакеты и ходы).
    void AgentGameSession::ClearPending()
    {
        std::lock_guard<std::mutex> lock(mutex);
        pendingItems.clear();
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

    // Канал: снимок счётчика завершений пакетов place.
    unsigned long long AgentGameSession::GetPlaceGeneration() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return placeGeneration;
    }

    // Игровой цикл: начат выполнение пакет place.
    void AgentGameSession::MarkPlaceStarted(unsigned int scoreBefore)
    {
        std::lock_guard<std::mutex> lock(mutex);
        placeScoreBefore = scoreBefore;
    }

    // Игровой цикл: пакет place завершён.
    void AgentGameSession::CompletePlace(int linesCleared, unsigned int scoreAfter)
    {
        std::lock_guard<std::mutex> lock(mutex);
        placeResult.linesCleared = linesCleared;
        placeResult.scoreDelta = scoreAfter - placeScoreBefore;
        ++placeGeneration;
    }

    // Канал: результат пакета place.
    AgentPlaceResult AgentGameSession::GetPlaceResult() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return placeResult;
    }
}
