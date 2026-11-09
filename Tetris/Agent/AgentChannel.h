#pragma once

#include "AgentGameSession.h"
#include "Json.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <winsock2.h>
#include <windows.h>

namespace Tetris
{
    // Агентский канал: TCP-сервер на 127.0.0.1 (WinAPI-сокеты), один клиент.
    // Протокол: строка = JSON (разделитель \n). Запускается в фоновом потоке.
    //
    // Команды клиента (агента/MCP-сервера):
    //   {"id":N,"type":"get_state"}            -> {"id":N,"type":"state", ...}
    //   {"id":N,"type":"get_rules"}            -> {"id":N,"type":"rules","rules":"..."}
    //   {"id":N,"type":"action","action":"left|right|rotate_up|rotate_down|hard_drop"}
    //                                          -> {"id":N,"type":"ok"}
    //   {"id":N,"type":"restart"}              -> {"id":N,"type":"ok"}
    //   {"id":N,"type":"quit"}                 -> {"id":N,"type":"ok"}
    //
    // Потокобезопасность: Start() работает в фоновом потоке и сам закрывает
    // свои сокеты при выходе. Stop() только ставит атомарный флаг — поток
    // замечает его через select с таймаутом и завершается (безопасно).
    class AgentChannel
    {
    private:
        SOCKET listenSocket{ INVALID_SOCKET };
        SOCKET clientSocket{ INVALID_SOCKET };
        std::atomic<bool> stopRequested{ false };
        std::atomic<int> boundPort{ -1 };
        bool quitRequested{ false };
        bool wsStarted{ false };
        AgentGameSession *session{ nullptr };

        // Отправляет строку клиенту (с \n). false при ошибке/отсутствии клиента.
        bool SendLine(const std::string &line);

        // Читает строку от клиента (до \n). false при разрыве/ошибке/стопе.
        bool ReadLine(std::string &line);

        // Заполняет JsonValue полями состояния (человекочитаемый формат).
        static void FillState(JsonValue &root, const GameStateSnapshot &state);

        // Обслуживает одну команду (JSON), отправляет ответ.
        void HandleCommand(const std::string &line);

    public:
        // Диапазон портов: 7777..7786 (первый свободный).
        static constexpr int kFirstPort = 7777;
        static constexpr int kPortCount = 10;

        AgentChannel();
        ~AgentChannel();

        // Привязывает канал к сессии игры (источник состояния и приёмник
        // действий). Вызывается до Start().
        void SetSession(AgentGameSession *gameSession);

        // Запускает сервер: bind/listen/accept. Возвращает порт (или -1 при
        // ошибке). Блокируется, пока не вызван Stop() (агентский поток).
        int Start();

        // Останавливает сервер: ставит флаг, поток завершится сам.
        void Stop();

        // Запрошен ли выход (команда quit).
        bool IsQuitRequested() const;

        // Порт, к которому канал привязался (-1, пока не привязан).
        // Основной поток опрашивает его, пока фоновый поток в Start().
        int GetBoundPort() const;

        // Правила игры и формат состояния (для LLM-агента).
        static std::string GetRulesText();
    };
}
