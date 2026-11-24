#include "McpServer.h"

#include "AgentChannel.h"
#include "Json.h"

#include <cstddef>
#include <chrono>
#include <fcntl.h>
#include <optional>
#include <io.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace Tetris
{
    namespace
    {
        // Диагностическое сообщение (только stderr: stdout — протокол MCP).
        void Log(const std::string &message)
        {
            std::cerr << "[mcp] " << message << "\n";
            std::cerr.flush();
        }

        // Строка протокола в stdout.
        void WriteLine(const std::string &line)
        {
            std::cout << line << "\n";
            std::cout.flush();
        }

        // Пустой JSON-объект.
        JsonValue MakeEmptyObject()
        {
            JsonValue value;
            value.type = JsonValue::Type::Object;
            return value;
        }

        // id запроса (число или строка; у уведомлений отсутствует).
        struct RequestId
        {
            bool present{ false };
            bool isString{ false };
            int64_t number{ 0 };
            std::string text;
        };

        RequestId ExtractId(const JsonValue &request)
        {
            RequestId result;
            const JsonValue *idValue = request.Find("id");

            if (idValue == nullptr)
            {
                return result;
            }

            result.present = true;

            if (idValue->type == JsonValue::Type::String)
            {
                result.isString = true;
                result.text = idValue->stringValue;
            }
            else
            {
                result.number = idValue->AsInt();
            }

            return result;
        }

        std::string MakeResult(const RequestId &id, const JsonValue &result)
        {
            JsonValue response;
            response["jsonrpc"] = std::string("2.0");

            if (id.isString)
            {
                response["id"] = id.text;
            }
            else
            {
                response["id"] = id.number;
            }

            response["result"] = result;
            return SerializeJson(response);
        }

        std::string MakeError(const RequestId &id, int code, const std::string &message)
        {
            JsonValue error;
            error["code"] = code;
            error["message"] = message;

            JsonValue response;
            response["jsonrpc"] = std::string("2.0");

            if (id.isString)
            {
                response["id"] = id.text;
            }
            else
            {
                response["id"] = id.number;
            }

            response["error"] = error;
            return SerializeJson(response);
        }

        // Определение инструмента (без параметров).
        JsonValue MakeTool(const char *name, const std::string &description)
        {
            JsonValue inputSchema;
            inputSchema["type"] = std::string("object");
            inputSchema["properties"] = MakeEmptyObject();

            JsonValue tool;
            tool["name"] = name;
            tool["description"] = description;
            tool["inputSchema"] = inputSchema;
            return tool;
        }

        // Определение инструмента place: inputSchema с массивом actions
        // (шаги расстановки; после пакета игра сама опускает фигуру до упора).
        JsonValue MakePlaceTool()
        {
            JsonValue actionNames;
            actionNames.type = JsonValue::Type::Array;
            actionNames.arrayValue.push_back(JsonValue("left"));
            actionNames.arrayValue.push_back(JsonValue("right"));
            actionNames.arrayValue.push_back(JsonValue("rotate_up"));
            actionNames.arrayValue.push_back(JsonValue("rotate_down"));
            actionNames.arrayValue.push_back(JsonValue("move_down"));
            actionNames.arrayValue.push_back(JsonValue("hard_drop"));

            JsonValue actionItems;
            actionItems["type"] = std::string("string");
            actionItems["enum"] = actionNames;

            JsonValue actionsSchema;
            actionsSchema["type"] = std::string("array");
            actionsSchema["items"] = actionItems;
            actionsSchema["minItems"] = 1;
            actionsSchema["maxItems"] = 64;

            JsonValue properties;
            properties["actions"] = actionsSchema;

            JsonValue required;
            required.type = JsonValue::Type::Array;
            required.arrayValue.push_back(JsonValue("actions"));

            JsonValue inputSchema;
            inputSchema["type"] = std::string("object");
            inputSchema["properties"] = properties;
            inputSchema["required"] = required;

            JsonValue tool;
            tool["name"] = std::string("place");

            tool["description"] =
                std::string(
                    "ОСНОВНОЙ ход: расставить текущую фигуру одной командой. "
                    "Поле actions — список шагов: left / right — сдвинуть фигуру на 1 колонку, "
                    "rotate_up — поворот против часовой стрелки, rotate_down — по часовой, "
                    "move_down — один ряд вниз, hard_drop — сразу до упора. "
                    "После выполнения пакета игра САМА опускает фигуру до упора и фиксирует её "
                    "(hard_drop не обязателен, но допускается в пакете). "
                    "В ответе — НОВОЕ состояние партии: board, features, current, next, score, level, "
                    "lines_cleared (стёрто линий за пакет), score_delta, game_over, win. "
                    "Стратегия выбора расстановки — инструмент get_rules.");

            tool["inputSchema"] = inputSchema;
            return tool;
        }

        // Список инструментов MCP (tools/list). Описания — для LLM-агента:
        // координаты поля, реальные направления поворотов, напоминание
        // о get_rules (агенту не должно быть нужно угадывать правила).
        JsonValue BuildToolsList()
        {
            JsonValue tools;
            tools.type = JsonValue::Type::Array;

            tools.arrayValue.push_back(MakeTool(
                "get_state",
                "Состояние партии. Поле 10 колонок (x: 0 слева, 9 справа) на 22 ряда (y: 0 сверху, 21 снизу). "
                "board — 22 строки по 10 символов: '.' — пусто, буква A..G — зафиксированная фигура. "
                "features — признаки поля: heights (высоты колонок 0..21), holes (закрытые дыры), "
                "bumpiness (неровность поверхности), aggregate_height (общая высота блоков) — "
                "для выбора расстановки, минимизирующей holes + bumpiness + aggregate_height. "
                "current — текущая падающая фигура: id (A..G) и cells — её клетки [x, y] в координатах поля. "
                "next — следующая фигура (id и cells). score, level, game_over, win. "
                "agent_running — запущена ли агентская игра. Символы: A=I (4x1), B=O (2x2), "
                "C=T, D=S, E=Z, F=J, G=L. Если игра не запущена (agent_running: false), "
                "сначала вызовите инструмент start. Правила игры — в инструменте get_rules."));

            tools.arrayValue.push_back(
                MakeTool("get_rules",
                    "Правила Тетриса и формат состояния: как играть, что означают поля get_state, "
                    "координаты, счёт, game over. Вызовите перед первой партией и при любом сомнении."));

            tools.arrayValue.push_back(MakePlaceTool());

            tools.arrayValue.push_back(MakeTool(
                "start",
                "Запустить агентскую игру (нужно выполнить ДО любого другого инструмента). "
                "Если игра уже запущена (порты 7777-7786), — просто подключиться к ней; "
                "иначе — запустить новый процесс Tetris.exe --agent в отдельном окне. "
                "После quit (завершения игры) игру можно запустить повторно — снова start."));

            tools.arrayValue.push_back(MakeTool(
                "move_left",
                "Сдвинуть текущую фигуру на 1 колонку влево (x уменьшается). Поле всего 10 колонок: "
                "от левого края до правого — 9 сдвигов. Столкновение со стеной или стеком игнорируется. "
                "Для игры лучше использовать инструмент place."));

            tools.arrayValue.push_back(MakeTool(
                "move_right", "Сдвинуть текущую фигуру на 1 колонку вправо (x увеличивается). Поле всего 10 колонок: "
                "от левого края до правого — 9 сдвигов. Столкновение со стеной или стеком игнорируется. "
                "Для игры лучше использовать инструмент place."));

            tools.arrayValue.push_back(MakeTool(
                "rotate_up",
                "Повернуть текущую фигуру ПРОТИВ часовой стрелки. Если поворот невозможен "
                "(стена, стек), фигура остаётся без изменений. "
                "Для игры лучше использовать инструмент place."));

            tools.arrayValue.push_back(MakeTool(
                "rotate_down",
                "Повернуть текущую фигуру ПО часовой стрелке. Если поворот невозможен "
                "(стена, стек), фигура остаётся без изменений. "
                "Для игры лучше использовать инструмент place."));

            tools.arrayValue.push_back(MakeTool(
                "move_down",
                "Опустить текущую фигуру на один ряд вниз (гравитации нет: без move_down "
                "или hard_drop фигура висит на месте). Не смогла упасть (упёрлась в "
                "пол или стек) — фигура фиксируется в стеке, начинается следующая. "
                "Для игры лучше использовать инструмент place."));

            tools.arrayValue.push_back(MakeTool(
                "hard_drop",
                "Мгновенно опустить текущую фигуру до упора: она фиксируется в стеке, "
                "сразу начинается следующая (см. next в get_state). "
                "Для игры лучше использовать инструмент place."));

            tools.arrayValue.push_back(MakeTool(
                "restart",
                "Начать новую партию (после game_over или победы)."));

            tools.arrayValue.push_back(MakeTool(
                "quit",
                "Завершить игру (процесс Tetris.exe)."));

            return tools;
        }

        // Клиент агентского канала игры (TCP 127.0.0.1, протокол «строка = JSON»).
        class AgentClient
        {
        private:
            SOCKET clientSocket{ INVALID_SOCKET };
            int64_t nextId{ 1 };
            bool wsStarted{ false };

            // Отправляет строку (с \n). false при ошибке.
            bool SendLine(const std::string &line)
            {
                const std::string payload = line + "\n";
                int totalSent = 0;

                while (totalSent < static_cast<int>(payload.size()))
                {
                    const int sent =
                        send(clientSocket, payload.c_str() + totalSent, static_cast<int>(payload.size()) - totalSent, 0);

                    if (sent <= 0)
                    {
                        return false;
                    }

                    totalSent += sent;
                }

                return true;
            }

            // Читает строку (до \n). false при разрыве/таймауте (5 с).
            bool ReadLine(std::string &line)
            {
                char buffer[4096];

                while (true)
                {
                    fd_set readSet;
                    FD_ZERO(&readSet);
                    FD_SET(clientSocket, &readSet);
                    timeval timeout{ 5, 0 };
                    const int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);

                    if (selectResult <= 0)
                    {
                        return false;
                    }

                    const int received = recv(clientSocket, buffer, sizeof(buffer), 0);

                    if (received <= 0)
                    {
                        return false;
                    }

                    for (int i = 0; i < received; ++i)
                    {
                        if (buffer[i] == '\n')
                        {
                            return true;
                        }

                        line += buffer[i];
                    }

                    if (line.size() > 65536)
                    {
                        return false;
                    }
                }
            }

        public:
            AgentClient()
            {
                WSADATA wsaData;

                if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
                {
                    wsStarted = true;
                }
            }

            ~AgentClient()
            {
                if (clientSocket != INVALID_SOCKET)
                {
                    (void)closesocket(clientSocket);
                }

                if (wsStarted)
                {
                    (void)WSACleanup();
                }
            }

            // Отключение от канала (после разрыва / quit игры).
            void Disconnect()
            {
                if (clientSocket != INVALID_SOCKET)
                {
                    (void)closesocket(clientSocket);
                    clientSocket = INVALID_SOCKET;
                }
            }

            // Подключено ли к агентскому каналу.
            bool IsConnected() const
            {
                return clientSocket != INVALID_SOCKET;
            }

            // Подключение к первому ответившему порту 7777-7786.
            bool Connect()
            {
                for (int port = AgentChannel::kFirstPort; port < AgentChannel::kFirstPort + AgentChannel::kPortCount;
                     ++port)
                {
                    SOCKET candidate = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                    if (candidate == INVALID_SOCKET)
                    {
                        continue;
                    }

                    sockaddr_in addr{};
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(static_cast<u_short>(port));
                    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                    if (connect(candidate, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0)
                    {
                        clientSocket = candidate;
                        return true;
                    }

                    (void)closesocket(candidate);
                }

                return false;
            }

            // Команда каналу игры и ответ (с тем же id).
            bool Call(const std::string &type, const JsonValue &extra, JsonValue &response)
            {
                JsonValue request;
                request["id"] = nextId++;
                request["type"] = type;

                for (const auto &field : extra.objectValue)
                {
                    request[field.first] = field.second;
                }

                if (!SendLine(SerializeJson(request)))
                {
                    return false;
                }

                std::string line;

                if (!ReadLine(line))
                {
                    return false;
                }

                return ParseJson(line, response);
            }
        };

        // PID агентской игры, запущенной сервером (команда start; 0 — не запущена).
        // Только свои процессы сервер останавливает; чужую игру (запущенную
        // пользователем вручную) он не трогает.
        std::optional<DWORD> spawnedPid;

        // Задействован ли хотя бы один агентский порт 7777-7786 (пробный connect).
        bool IsAgentPortBusy()
        {
            for (int port = AgentChannel::kFirstPort; port < AgentChannel::kFirstPort + AgentChannel::kPortCount;
                 ++port)
            {
                SOCKET candidate = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                if (candidate == INVALID_SOCKET)
                {
                    continue;
                }

                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(static_cast<u_short>(port));
                addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                const bool busy = connect(candidate, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0;
                (void)closesocket(candidate);

                if (busy)
                {
                    return true;
                }
            }

            return false;
        }

        // Запуск режима агента инструментом start: Tetris.exe --agent из каталога MCP-сервера.
        // Агенту своё видимое окно (CREATE_NEW_CONSOLE): партию можно наблюдать
        // вживую, а рендер не конфликтует со stdout/stdin MCP-сервера (у агента
        // отдельная консоль, а не пайпы клиента).
        bool SpawnAgent()
        {
            wchar_t modulePath[MAX_PATH]{};
            const DWORD pathLength = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

            if (pathLength == 0 || pathLength >= MAX_PATH)
            {
                return false;
            }

            std::wstring commandLine;
            commandLine += L'"';
            commandLine += modulePath;
            commandLine += L"\" --agent";

            std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
            commandBuffer.push_back(L'\0');

            STARTUPINFOW startupInfo{};
            startupInfo.cb = sizeof(startupInfo);
            PROCESS_INFORMATION processInfo{};

            const BOOL created = CreateProcessW(
                modulePath,
                commandBuffer.data(),
                nullptr,
                nullptr,
                false,
                CREATE_NEW_CONSOLE,
                nullptr,
                nullptr,
                &startupInfo,
                &processInfo);

            if (!created)
            {
                return false;
            }

            // Живём в фоне: хэндлы процесса сохраняем, чтобы при выходе
            // MCP-сервера остановить свою игру (TerminateProcess).
            spawnedPid = processInfo.dwProcessId;
            (void)CloseHandle(processInfo.hThread);

            std::string modulePathUtf8;
            const int pathChars =
                WideCharToMultiByte(CP_UTF8, 0, modulePath, static_cast<int>(pathLength), nullptr, 0, nullptr, nullptr);

            if (pathChars > 0)
            {
                modulePathUtf8.resize(static_cast<size_t>(pathChars));
                (void)WideCharToMultiByte(
                    CP_UTF8, 0, modulePath, static_cast<int>(pathLength), modulePathUtf8.data(), pathChars, nullptr, nullptr);
            }

            Log("запущен режим агента: " + modulePathUtf8 + " --agent");
            return true;
        }

        // Ожидание появления агентского канала: повторное подключение
        // каждые 200 мс до дедлайна.
        bool WaitAgentChannel(AgentClient &client, std::chrono::milliseconds wait)
        {
            const auto deadline = std::chrono::steady_clock::now() + wait;

            while (std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                if (client.Connect())
                {
                    return true;
                }
            }

            return false;
        }

        // Команда start: подключиться к уже запущенной игре либо запустить
        // свою (Tetris.exe --agent в своём видимом окне) и дождаться канала.
        bool ConnectOrSpawnAgent(AgentClient &client)
        {
            if (client.Connect())
            {
                Log("подключено к уже запущенной агентской игре");
                return true;
            }

            if (!IsAgentPortBusy() && !SpawnAgent())
            {
                Log("запуск режима агента не удался (CreateProcessW: " +
                    std::to_string(GetLastError()) + ")");
                return false;
            }
            // Порт занят, но никто не отвечает (зависшая чужая игра) —
            // второго экземпляра не поднимаем, просто ждём её.

            if (!WaitAgentChannel(client, std::chrono::seconds(10)))
            {
                Log("агентский канал не появился (10 с) — агент не стартовал или все порты 7777-7786 заняты");
                return false;
            }

            Log("подключено к агентской игре");
            return true;
        }

        // Остановить агентскую игру, запущенную сервером (командой start),
        // если она ещё жива. Чужие игры не трогаем.
        void TerminateSpawnedAgent()
        {
            if (spawnedPid.has_value())
            {
                HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, spawnedPid.value());

                if (processHandle != nullptr)
                {
                    (void)TerminateProcess(processHandle, 0);
                    (void)WaitForSingleObject(processHandle, 5000);
                    (void)CloseHandle(processHandle);
                    Log("остановлена агентская игра, запущенная MCP-сервером");
                }

                spawnedPid.reset();
            }
        }
    } // namespace

    // MCP-сервер (режим --mcp-server): stdio JSON-RPC 2.0 по спецификации MCP.
    // Входящие запросы — из stdin, исходящие ответы — в stdout (строка = JSON).
    // Сервер сам подключается к агентскому каналу игры (TCP 127.0.0.1,
    // порты 7777-7786, первый ответивший) и проксирует вызовы инструментов.
    //
    // Все диагностические сообщения — только в stderr (stdout — протокол).
    // Возвращает код выхода: 0 — штатное завершение, 1 — ошибка.
    int RunMcpServer()
    {
        // stdio в двоичном режиме (JSON — ASCII, но без перевода \n в \r\n).
        (void)_setmode(_fileno(stdin), _O_BINARY);
        (void)_setmode(_fileno(stdout), _O_BINARY);

        // Запущен MCP-клиентом (stdin — пайп): видимое консольное окно не
        // оставляем — сервер живёт в фоне. Окно скрываем только если консоль
        // создана под нас (в ней один процесс — мы; общую консоль, в которую
        // запущен человек, не трогаем). Консоли нет вовсе (stdio — пайпы,
        // окно не создавалось) — GetConsoleWindow() вернёт NULL, скрывать
        // нечего. Кодовую страницу вывода ставим UTF-8: лог в stderr —
        // UTF-8 из /utf-8-исходников, а кодовая страница консоли печатала
        // его кракозябрами. При stdin из консоли (запуск человеком) —
        // ничего не трогаем.
        if (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_PIPE)
        {
            (void)SetConsoleOutputCP(CP_UTF8);

            DWORD consoleProcesses[4]{};
            const DWORD processCount = GetConsoleProcessList(consoleProcesses, 4);

            if (processCount == 1)
            {
                const HWND consoleWindow = GetConsoleWindow();

                if (consoleWindow != nullptr)
                {
                    (void)ShowWindow(consoleWindow, SW_HIDE);
                }
            }
        }

        AgentClient client;

        // Игра не запускается и не подключается автоматически: режим агента
        // поднимает инструмент start, сервер просто ожидает запросы MCP-клиента.
        Log("ожидание запросов; агентская игра не запущена (инструмент start — запускает игру)");

        std::string line;

        while (std::getline(std::cin, line))
        {
            if (line.empty())
            {
                continue;
            }

            JsonValue request;

            if (!ParseJson(line, request))
            {
                RequestId emptyId;
                WriteLine(MakeError(emptyId, -32700, "Parse error"));
                continue;
            }

            const RequestId id = ExtractId(request);
            const JsonValue *methodValue = request.Find("method");
            const std::string method = methodValue != nullptr ? methodValue->AsString() : std::string();

            if (method == "initialize")
            {
                JsonValue result;
                result["protocolVersion"] = std::string("2025-06-18");

                JsonValue serverInfo;
                serverInfo["name"] = std::string("tetris-mcp");
                serverInfo["version"] = std::string("1.0.0");
                result["serverInfo"] = serverInfo;

                JsonValue toolsCapability;
                toolsCapability["listChanged"] = false;
                JsonValue capabilities;
                capabilities["tools"] = toolsCapability;
                result["capabilities"] = capabilities;

                if (id.present)
                {
                    WriteLine(MakeResult(id, result));
                }
            }
            else if (method.rfind("notifications/", 0) == 0)
            {
                // Уведомления MCP — без ответа.
            }
            else if (method == "tools/list")
            {
                JsonValue result;
                result["tools"] = BuildToolsList();

                if (id.present)
                {
                    WriteLine(MakeResult(id, result));
                }
            }
            else if (method == "tools/call")
            {
                const JsonValue *paramsValue = request.Find("params");
                const JsonValue *nameValue = paramsValue != nullptr ? paramsValue->Find("name") : nullptr;
                const std::string toolName = nameValue != nullptr ? nameValue->AsString() : std::string();

                std::string agentType;
                std::string agentAction;
                JsonValue extra;

                if (toolName == "get_state")
                {
                    agentType = "get_state";
                }
                else if (toolName == "get_rules")
                {
                    agentType = "get_rules";
                }
                else if (toolName == "move_left")
                {
                    agentType = "action";
                    agentAction = "left";
                }
                else if (toolName == "move_right")
                {
                    agentType = "action";
                    agentAction = "right";
                }
                else if (toolName == "rotate_up")
                {
                    agentType = "action";
                    agentAction = "rotate_up";
                }
                else if (toolName == "rotate_down")
                {
                    agentType = "action";
                    agentAction = "rotate_down";
                }
                else if (toolName == "move_down")
                {
                    agentType = "action";
                    agentAction = "move_down";
                }
                else if (toolName == "hard_drop")
                {
                    agentType = "action";
                    agentAction = "hard_drop";
                }
                else if (toolName == "restart")
                {
                    agentType = "restart";
                }
                else if (toolName == "quit")
                {
                    agentType = "quit";
                }
                else if (toolName == "place")
                {
                    agentType = "place";

                    const JsonValue *argsValue = paramsValue != nullptr ? paramsValue->Find("arguments") : nullptr;
                    const JsonValue *actionsValue = argsValue != nullptr ? argsValue->Find("actions") : nullptr;

                    if (actionsValue == nullptr || actionsValue->type != JsonValue::Type::Array ||
                        actionsValue->arrayValue.empty())
                    {
                        if (id.present)
                        {
                            WriteLine(MakeError(id, -32602, "place: параметр actions — непустой массив шагов"));
                        }

                        continue;
                    }

                    extra["actions"] = *actionsValue;
                }
                else if (toolName == "start")
                {
                    // Локальная команда: запускает агентскую игру, если она не запущена,
                    // иначе подключается к уже запущенной.
                    JsonValue startResult;
                    startResult["agent_running"] = ConnectOrSpawnAgent(client);

                    if (id.present)
                    {
                        WriteLine(MakeResult(id, startResult));
                    }

                    continue;
                }
                else
                {
                    if (id.present)
                    {
                        WriteLine(MakeError(id, -32602, "Неизвестный инструмент: " + toolName));
                    }

                    continue;
                }

                if (!agentAction.empty())
                {
                    extra["action"] = agentAction;
                }

                // Игра не запущена — сначала инструмент start.
                if (!client.IsConnected())
                {
                    if (id.present)
                    {
                        WriteLine(MakeError(
                            id,
                            -32603,
                            "Агентская игра не запущена. Сначала вызовите инструмент start."));
                    }

                    continue;
                }

                JsonValue agentResponse;

                if (!client.Call(agentType, extra, agentResponse))
                {
                    client.Disconnect();

                    if (id.present)
                    {
                        WriteLine(MakeError(
                            id,
                            -32603,
                            "Агентская игра отключилась (завершена?). Запустите её инструментом start."));
                    }

                    continue;
                }

                const JsonValue *typeValue = agentResponse.Find("type");
                const std::string responseType = typeValue != nullptr ? typeValue->AsString() : std::string();

                if (responseType == "error")
                {
                    const JsonValue *errorValue = agentResponse.Find("error");
                    const std::string error = errorValue != nullptr ? errorValue->AsString() : std::string("ошибка");

                    if (id.present)
                    {
                        WriteLine(MakeError(id, -32603, "Игра: " + error));
                    }

                    continue;
                }

                // Игра завершилась по quit (канал закрыл соединение) —
                // сбрасываем подключение: агент может запустить новую игру
                // инструментом start.
                if (agentType == "quit")
                {
                    client.Disconnect();

                    if (spawnedPid.has_value())
                    {
                        const HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, spawnedPid.value());

                        if (processHandle != nullptr)
                        {
                            DWORD exitCode = 0;

                            if (GetExitCodeProcess(processHandle, &exitCode) &&
                                exitCode != STILL_ACTIVE)
                            {
                                (void)CloseHandle(processHandle);
                                spawnedPid.reset();
                            }
                            else
                            {
                                (void)CloseHandle(processHandle);
                            }
                        }
                    }
                }

                // Результат инструмента MCP: content — массив текстовых элементов.
                JsonValue content;
                content.type = JsonValue::Type::Array;

                JsonValue textItem;
                textItem["type"] = std::string("text");
                textItem["text"] = SerializeJson(agentResponse);
                content.arrayValue.push_back(textItem);

                JsonValue result;
                result["content"] = content;
                result["isError"] = false;
                result["agent_running"] = client.IsConnected();

                if (id.present)
                {
                    WriteLine(MakeResult(id, result));
                }
            }
            else if (method == "ping")
            {
                if (id.present)
                {
                    WriteLine(MakeResult(id, MakeEmptyObject()));
                }
            }
            else
            {
                if (id.present)
                {
                    WriteLine(MakeError(id, -32601, "Метод не найден: " + method));
                }
            }
        }

        // При выходе останавливаем агентскую игру, если её запустили мы
        // (команда start). Чужую (запущенную пользователем вручную) не трогаем.
        if (client.IsConnected())
        {
            client.Disconnect();
        }

        TerminateSpawnedAgent();

        Log("stdin закрыт — MCP-сервер завершается");
        return 0;
    }
}
