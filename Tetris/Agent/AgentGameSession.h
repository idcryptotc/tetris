#pragma once

#include "../Core/TetrisEngine.h"

#include <deque>
#include <mutex>
#include <optional>
#include <vector>

namespace Tetris
{
    // Пакет ходов: одиночное действие агента (команда action) или полная
    // расстановка фигуры (команда place: все действия одной командой).
    // Игровой цикл исполняет ходы пакета по порядку; для пакета place
    // затем сам опускает фигуру до упора (hard_drop) и фиксирует её.
    struct AgentActionBatch
    {
        std::vector<PlayerAction> actions;
        bool isPlace{ false }; // true — пакет place
    };

    // Результат выполнения пакета place (игровой цикл → канал): канал
    // ждёт завершения пакета и возвращает его в ответе place вместе с
    // новым состоянием партии.
    struct AgentPlaceResult
    {
        int linesCleared{ 0 };         // линий, стёртых после фиксации фигуры
        unsigned int scoreDelta{ 0 };  // прирост счёта за пакет
    };

    // Общее состояние между игровым циклом (основной поток) и агентским
    // каналом (фоновый поток). Движок (TetrisEngine) живёт в игровом цикле
    // и изменяется только им; этот класс хранит лишь данные, которыми
    // обмениваются потоки (очередь ходов и пакетов, снимок состояния, флаги),
    // защищённые мьютексом.
    class AgentGameSession
    {
    private:
        mutable std::mutex mutex;
        std::deque<AgentActionBatch> pendingItems;
        GameStateSnapshot currentState;
        bool running{ true };
        bool restartRequested{ false };

        // Завершение пакета place: счётчик (инкремент при каждом завершении
        // пакета) и результат выполнения. Канал снимает счётчик до постановки
        // пакета в очередь и опрашивает его, пока тот не изменится (пакет
        // выполнен).
        unsigned long long placeGeneration{ 0 };
        AgentPlaceResult placeResult;
        unsigned int placeScoreBefore{ 0 };

        // Забирает следующий элемент очереди (одиночный ход или пакет) и
        // очищает её; если пустая — nullopt.
        std::optional<AgentActionBatch> TakeItem()
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (pendingItems.empty())
            {
                return std::nullopt;
            }

            const AgentActionBatch item = std::move(pendingItems.front());
            pendingItems.clear();
            return item;
        }

    public:
        // Агент: добавить действие в очередь (игровой цикл применит его).
        void EnqueueAction(PlayerAction action);

        // Агент: добавить пакет place в очередь (полная расстановка фигуры).
        void EnqueuePlace(std::vector<PlayerAction> actions);

        // Игровой цикл: забрать следующий элемент очереди (при отсутствии —
        // nullopt), очистив очередь.
        std::optional<AgentActionBatch> TakeNextItem();

        // Игровой цикл: очистить очередь (пакеты и ходы) — например, при
        // рестарте, чтобы накопленное не применялось к новой партии.
        void ClearPending();

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

        // Канал: снимок счётчика завершений пакетов place (для ожидания
        // выполнения пакета: изменение — пакет завершён).
        unsigned long long GetPlaceGeneration() const;

        // Игровой цикл: начат выполнение пакет place (scoreBefore — счёт на
        // момент начала пакета, для прироста в результате).
        void MarkPlaceStarted(unsigned int scoreBefore);

        // Игровой цикл: пакет place завершён (фигура зафиксирована, линии
        // очищены, новая фигура в игре): записывает результат и инкрементирует
        // счётчик — на это ждёт канал.
        void CompletePlace(int linesCleared, unsigned int scoreAfter);

        // Канал: результат пакета place (актуален после изменения счётчика).
        AgentPlaceResult GetPlaceResult() const;
    };
}
