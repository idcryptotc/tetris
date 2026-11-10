#include "McpServer.h"

#include "AgentChannel.h"
#include "Json.h"

#include <cstddef>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

namespace Tetris {
namespace {
    // Диагностическое сообщение (только stderr: stdout — протокол MCP).
    void Log(const std::string &message) {
        std::cerr << "[mcp] " << message << "\n";
        std::cerr.flush();
    }

    // Строка протокола в stdout.
    void WriteLine(const std::string &line) {
        std::cout << line << "\n";
        std::cout.flush();
    }

    // Пустой JSON-объект.
    JsonValue MakeEmptyObject() {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        return value;
    }

    // id запроса (число или строка; у уведомлений отсутствует).
    struct RequestId {
        bool present{false};
        bool isString{false};
        int64_t number{0};
        std::string text;
    };

    RequestId ExtractId(const JsonValue &request) {
        RequestId result;
        const JsonValue *idValue = request.Find("id");

        if (idValue == nullptr) {
            return result;
        }

        result.present = true;

        if (idValue->type == JsonValue::Type::String) {
            result.isString = true;
            result.text = idValue->stringValue;
        } else {
            result.number = idValue->AsInt();
        }

        return result;
    }

    std::string MakeResult(const RequestId &id, const JsonValue &result) {
        JsonValue response;
        response["jsonrpc"] = std::string("2.0");

        if (id.isString) {
            response["id"] = id.text;
        } else {
            response["id"] = id.number;
        }

        response["result"] = result;
        return SerializeJson(response);
    }

    std::string MakeError(const RequestId &id, int code, const std::string &message) {
        JsonValue error;
        error["code"] = code;
        error["message"] = message;

        JsonValue response;
        response["jsonrpc"] = std::string("2.0");

        if (id.isString) {
            response["id"] = id.text;
        } else {
            response["id"] = id.number;
        }

        response["error"] = error;
        return SerializeJson(response);
    }

    // Определение тулза (без параметров).
    JsonValue MakeTool(const char *name, const std::string &description) {
        JsonValue inputSchema;
        inputSchema["type"] = std::string("object");
        inputSchema["properties"] = MakeEmptyObject();

        JsonValue tool;
        tool["name"] = name;
        tool["description"] = description;
        tool["inputSchema"] = inputSchema;
        return tool;
    }

    // Список тулзов MCP (tools/list). Описания — для LLM-агента:
    // координаты поля, реальные направления поворотов, напоминание
    // о get_rules (агенту не должно быть нужно угадывать правила).
    JsonValue BuildToolsList() {
        JsonValue tools;
        tools.type = JsonValue::Type::Array;
        tools.arrayValue.push_back(MakeTool(
            "get_state", "Состояние партии. Поле 10 колонок (x: 0 слева, 9 справа) на 22 ряда (y: 0 сверху, 21 снизу). "
                         "board — 22 строки по 10 символов: '.' — пусто, буква A..G — зафиксированная фигура. "
                         "current — текущая падающая фигура: id (A..G) и cells — её клетки [x, y] в координатах поля. "
                         "next — следующая фигура (id и cells). score, level, game_over, win. "
                         "Символы: A=I (4x1), B=O (2x2), C=T, D=S, E=Z, F=J, G=L. "
                         "Правила игры — в тулзе get_rules."));
        tools.arrayValue.push_back(
            MakeTool("get_rules", "Правила Тетриса и формат состояния: как играть, что означают поля get_state, "
                                  "координаты, счёт, game over. Вызовите перед первой партией и при любом сомнении."));
        tools.arrayValue.push_back(MakeTool(
            "move_left", "Сдвинуть текущую фигуру на 1 колонку влево (x уменьшается). Поле всего 10 колонок: "
                         "от левого края до правого — 9 сдвигов. Столкновение со стеной или стеком игнорируется."));
        tools.arrayValue.push_back(MakeTool(
            "move_right", "Сдвинуть текущую фигуру на 1 колонку вправо (x увеличивается). Поле всего 10 колонок: "
                          "от левого края до правого — 9 сдвигов. Столкновение со стеной или стеком игнорируется."));
        tools.arrayValue.push_back(MakeTool("rotate_up",
                                            "Повернуть текущую фигуру ПРОТИВ часовой стрелки. Если поворот невозможен "
                                            "(стена, стек), фигура остаётся без изменений."));
        tools.arrayValue.push_back(MakeTool("rotate_down",
                                            "Повернуть текущую фигуру ПО часовой стрелке. Если поворот невозможен "
                                            "(стена, стек), фигура остаётся без изменений."));
        tools.arrayValue.push_back(MakeTool("move_down",
                                            "Опустить текущую фигуру на один ряд вниз (гравитации нет: без move_down "
                                            "или hard_drop фигура висит на месте). Не смогла упасть (упёрлась в "
                                            "пол или стек) — фигура фиксируется в стеке, начинается следующая."));
        tools.arrayValue.push_back(MakeTool("hard_drop",
                                            "Мгновенно опустить текущую фигуру до упора: она фиксируется в стеке, "
                                            "сразу начинается следующая (см. next в get_state)."));
        tools.arrayValue.push_back(MakeTool("restart", "Начать новую партию (после game_over или победы)."));
        tools.arrayValue.push_back(MakeTool("quit", "Завершить игру (процесс Tetris.exe)."));
        return tools;
    }

    // Клиент агентского канала игры (TCP 127.0.0.1, протокол «строка = JSON»).
    class AgentClient {
        private:
        SOCKET clientSocket{INVALID_SOCKET};
        int64_t nextId{1};
        bool wsStarted{false};

        // Отправляет строку (с \n). false при ошибке.
        bool SendLine(const std::string &line) {
            const std::string payload = line + "\n";
            int totalSent = 0;

            while (totalSent < static_cast<int>(payload.size())) {
                const int sent =
                    send(clientSocket, payload.c_str() + totalSent, static_cast<int>(payload.size()) - totalSent, 0);

                if (sent <= 0) {
                    return false;
                }

                totalSent += sent;
            }

            return true;
        }

        // Читает строку (до \n). false при разрыве/таймауте (5 с).
        bool ReadLine(std::string &line) {
            char buffer[4096];

            while (true) {
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(clientSocket, &readSet);
                timeval timeout{5, 0};
                const int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);

                if (selectResult <= 0) {
                    return false;
                }

                const int received = recv(clientSocket, buffer, sizeof(buffer), 0);

                if (received <= 0) {
                    return false;
                }

                for (int i = 0; i < received; ++i) {
                    if (buffer[i] == '\n') {
                        return true;
                    }

                    line += buffer[i];
                }

                if (line.size() > 65536) {
                    return false;
                }
            }
        }

        public:
        AgentClient() {
            WSADATA wsaData;

            if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
                wsStarted = true;
            }
        }

        ~AgentClient() {
            if (clientSocket != INVALID_SOCKET) {
                (void)closesocket(clientSocket);
            }

            if (wsStarted) {
                (void)WSACleanup();
            }
        }

        // Подключение к первому ответившему порту 7777-7786.
        bool Connect() {
            for (int port = AgentChannel::kFirstPort; port < AgentChannel::kFirstPort + AgentChannel::kPortCount;
                 ++port) {
                SOCKET candidate = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                if (candidate == INVALID_SOCKET) {
                    continue;
                }

                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(static_cast<u_short>(port));
                addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

                if (connect(candidate, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0) {
                    clientSocket = candidate;
                    return true;
                }

                (void)closesocket(candidate);
            }

            return false;
        }

        // Команда каналу игры и ответ (с тем же id).
        bool Call(const std::string &type, const JsonValue &extra, JsonValue &response) {
            JsonValue request;
            request["id"] = nextId++;
            request["type"] = type;

            for (const auto &field : extra.objectValue) {
                request[field.first] = field.second;
            }

            if (!SendLine(SerializeJson(request))) {
                return false;
            }

            std::string line;

            if (!ReadLine(line)) {
                return false;
            }

            return ParseJson(line, response);
        }
    };
} // namespace

// MCP-сервер (режим --mcp-server): stdio JSON-RPC 2.0 по спецификации MCP.
// Входящие запросы — из stdin, исходящие ответы — в stdout (строка = JSON).
// Сервер сам подключается к агентскому каналу игры (TCP 127.0.0.1,
// порты 7777-7786, первый ответивший) и проксирует вызовы тулзов.
//
// Все диагностические сообщения — только в stderr (stdout — протокол).
// Возвращает код выхода: 0 — штатное завершение, 1 — ошибка.
int RunMcpServer() {
    // stdio в двоичном режиме (JSON — ASCII, но без перевода \n в \r\n).
    (void)_setmode(_fileno(stdin), _O_BINARY);
    (void)_setmode(_fileno(stdout), _O_BINARY);

    AgentClient client;

    if (!client.Connect()) {
        Log("не удалось подключиться к агентскому каналу игры (порты 7777-7786); запущена ли игра с --agent?");
        return 1;
    }

    Log("подключено к агентскому каналу игры");

    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }

        JsonValue request;

        if (!ParseJson(line, request)) {
            RequestId emptyId;
            WriteLine(MakeError(emptyId, -32700, "Parse error"));
            continue;
        }

        const RequestId id = ExtractId(request);
        const JsonValue *methodValue = request.Find("method");
        const std::string method = methodValue != nullptr ? methodValue->AsString() : std::string();

        if (method == "initialize") {
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

            if (id.present) {
                WriteLine(MakeResult(id, result));
            }
        } else if (method.rfind("notifications/", 0) == 0) {
            // Уведомления MCP — без ответа.
        } else if (method == "tools/list") {
            JsonValue result;
            result["tools"] = BuildToolsList();

            if (id.present) {
                WriteLine(MakeResult(id, result));
            }
        } else if (method == "tools/call") {
            const JsonValue *paramsValue = request.Find("params");
            const JsonValue *nameValue = paramsValue != nullptr ? paramsValue->Find("name") : nullptr;
            const std::string toolName = nameValue != nullptr ? nameValue->AsString() : std::string();

            std::string agentType;
            std::string agentAction;

            if (toolName == "get_state") {
                agentType = "get_state";
            } else if (toolName == "get_rules") {
                agentType = "get_rules";
            } else if (toolName == "move_left") {
                agentType = "action";
                agentAction = "left";
            } else if (toolName == "move_right") {
                agentType = "action";
                agentAction = "right";
            } else if (toolName == "rotate_up") {
                agentType = "action";
                agentAction = "rotate_up";
            } else if (toolName == "rotate_down") {
                agentType = "action";
                agentAction = "rotate_down";
            } else if (toolName == "move_down") {
                agentType = "action";
                agentAction = "move_down";
            } else if (toolName == "hard_drop") {
                agentType = "action";
                agentAction = "hard_drop";
            } else if (toolName == "restart") {
                agentType = "restart";
            } else if (toolName == "quit") {
                agentType = "quit";
            } else {
                if (id.present) {
                    WriteLine(MakeError(id, -32602, "Неизвестный тулз: " + toolName));
                }

                continue;
            }

            JsonValue extra;

            if (!agentAction.empty()) {
                extra["action"] = agentAction;
            }

            JsonValue agentResponse;

            if (!client.Call(agentType, extra, agentResponse)) {
                if (id.present) {
                    WriteLine(MakeError(id, -32603, "Агентский канал: ошибка соединения (игра завершена?)"));
                }

                continue;
            }

            const JsonValue *typeValue = agentResponse.Find("type");
            const std::string responseType = typeValue != nullptr ? typeValue->AsString() : std::string();

            if (responseType == "error") {
                const JsonValue *errorValue = agentResponse.Find("error");
                const std::string error = errorValue != nullptr ? errorValue->AsString() : std::string("ошибка");

                if (id.present) {
                    WriteLine(MakeError(id, -32603, "Игра: " + error));
                }

                continue;
            }

            // Результат тулза MCP: content — массив текстовых элементов.
            JsonValue content;
            content.type = JsonValue::Type::Array;

            JsonValue textItem;
            textItem["type"] = std::string("text");
            textItem["text"] = SerializeJson(agentResponse);
            content.arrayValue.push_back(textItem);

            JsonValue result;
            result["content"] = content;
            result["isError"] = false;

            if (id.present) {
                WriteLine(MakeResult(id, result));
            }
        } else if (method == "ping") {
            if (id.present) {
                WriteLine(MakeResult(id, MakeEmptyObject()));
            }
        } else {
            if (id.present) {
                WriteLine(MakeError(id, -32601, "Метод не найден: " + method));
            }
        }
    }

    Log("stdin закрыт — MCP-сервер завершается");
    return 0;
}
} // namespace Tetris
