#include "AgentChannel.h"

#include "Json.h"

#include <cstddef>
#include <cwchar>
#include <stdio.h>
#include <string>
#include <ws2tcpip.h>

namespace Tetris
{
    // Лимит размера лога действий агента (байты): файл tetris_agent.log
    // рядом с exe; при открытии файл крупнее лимита очищается (в рамках
    // одной сессии файла не переоткрываем — лог не растёт бесконечно).
    static constexpr std::size_t kAgentLogMaxBytes = 512 * 1024;

    AgentChannel::AgentChannel()
    {
        WSADATA wsaData;

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
        {
            wsStarted = true;
        }
    }

    AgentChannel::~AgentChannel()
    {
        Stop();

        if (wsStarted)
        {
            (void)WSACleanup();
        }
    }

    void AgentChannel::SetSession(AgentGameSession *gameSession)
    {
        session = gameSession;
    }

    // Включает лог действий агента (флаг --agent-diag): ставит флаг и
    // открывает файл. Вызывается из основного потока, до запуска фоновой
    // нити канала — дальше в файл пишет только та нить.
    void AgentChannel::EnableLog()
    {
        logEnabled = true;
        OpenLog();
    }

    // Открывает лог-файл tetris_agent.log рядом с exe (каталог
    // исполняемого файла, GetModuleFileNameW): append; файл крупнее
    // лимита размера при открытии очищается (ротация без копирования).
    void AgentChannel::OpenLog()
    {
        if (!logEnabled || logFile != nullptr)
        {
            return;
        }

        wchar_t modulePath[MAX_PATH] = { 0 };
        const DWORD pathLength = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

        if (pathLength == 0 || pathLength == MAX_PATH)
        {
            return; // путь к exe не определили — лог недоступен
        }

        wchar_t *slash = wcsrchr(modulePath, L'\\');

        if (slash != nullptr)
        {
            *slash = L'\0';
        }

        const std::wstring logPath = std::wstring(modulePath) + L"\\tetris_agent.log";

        // Ротация: файл крупнее лимита очищается (старые записи не нужны).
        bool needsTruncate = false;
        FILE *probe = nullptr;
        const errno_t probeErr = _wfopen_s(&probe, logPath.c_str(), L"rb");

        if (probeErr == 0 && probe != nullptr)
        {
            (void)fseek(probe, 0, SEEK_END);
            const long size = ftell(probe);
            (void)fclose(probe);
            needsTruncate = size > static_cast<long>(kAgentLogMaxBytes);
        }

        const errno_t openErr = _wfopen_s(&logFile, logPath.c_str(), needsTruncate ? L"w" : L"a");

        if (openErr != 0 || logFile == nullptr)
        {
            logFile = nullptr; // путь недоступен (нет каталога/права) — лог выключен
        }
    }

    // Закрывает лог-файл. Вызывается из основного потока после join
    // фоновой нити канала (нить больше не пишет в файл).
    void AgentChannel::CloseLog()
    {
        if (logFile != nullptr)
        {
            (void)fclose(logFile);
            logFile = nullptr;
        }
    }

    // Записывает событие в лог (если файл открыт): время, направление
    // (-> — запрос, <- — ответ), тип, id, краткое состояние.
    void AgentChannel::LogEvent(bool isRequest, const std::string &type, int64_t id, const std::string &summary)
    {
        if (logFile == nullptr)
        {
            return;
        }

        SYSTEMTIME now;
        GetLocalTime(&now);

        const std::string suffix = summary.empty() ? std::string() : " " + summary;

        (void)fprintf(
            logFile,
            "%04d-%02d-%02d %02d:%02d:%02d %s %s id=%lld%s\n",
            static_cast<int>(now.wYear),
            static_cast<int>(now.wMonth),
            static_cast<int>(now.wDay),
            static_cast<int>(now.wHour),
            static_cast<int>(now.wMinute),
            static_cast<int>(now.wSecond),
            isRequest ? "->" : "<-",
            type.empty() ? "?" : type.c_str(),
            static_cast<long long>(id),
            suffix.c_str());
        (void)fflush(logFile);
    }

    // Отправляет строку клиенту (с \n). false при ошибке/отсутствии клиента.
    bool AgentChannel::SendLine(const std::string &line)
    {
        if (clientSocket == INVALID_SOCKET)
        {
            return false;
        }

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

    // Читает строку от клиента (до \n). false при разрыве/ошибке/стопе.
    bool AgentChannel::ReadLine(std::string &line)
    {
        char buffer[4096];

        while (true)
        {
            if (stopRequested.load())
            {
                return false;
            }

            // select с таймаутом — чтобы реагировать на Stop() и не виснуть
            // вечно в recv.
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(clientSocket, &readSet);
            timeval timeout{ 0, 500000 }; // 0.5 с
            const int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);

            if (selectResult < 0)
            {
                return false;
            }

            if (selectResult == 0)
            {
                continue; // таймаут — проверяем стоп и ждём дальше
            }

            const int received = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (received <= 0)
            {
                return false; // разрыв соединения
            }

            for (int i = 0; i < received; ++i)
            {
                if (buffer[i] == '\n')
                {
                    return true;
                }

                line += buffer[i];
            }

            // Защита от переполнения, если клиент не шлёт \n.
            if (line.size() > 65536)
            {
                return false;
            }
        }
    }

    // Правила игры и формат состояния (для LLM-агента). Статика: текст
    // константный, строка создаётся при каждом вызове — потокобезопасно.
    std::string AgentChannel::GetRulesText()
    {
        return
            "Тетрис. Цель — складывать фигуры так, чтобы заполнять целые горизонтальные линии: "
            "заполненная линия исчезает, блоки сверху падают, начисляются очки. "
            "Правила: "
            "1) Поле 10 колонок (x: 0 слева, 9 справа) на 22 ряда (y: 0 сверху, 21 снизу). "
            "2) Гравитации НЕТ: темп партии — ваш темп ходов: спешить не нужно. "
            "3) ОСНОВНОЙ ход — place: одна команда = полная расстановка фигуры. "
            "Поле actions — список шагов: left / right — сдвинуть текущую фигуру на 1 колонку "
            "влево/вправо (поле всего 10 колонок: чтобы пересечь его, нужно 9 сдвигов, не больше); "
            "rotate_up — поворот против часовой стрелки, rotate_down — по часовой; "
            "move_down — опустить фигуру на один ряд вниз; hard_drop — сразу до упора. "
            "После выполнения пакета игра САМА опускает фигуру до упора и фиксирует её — "
            "ход для фиксации не обязателен, но hard_drop можно включить в пакет. "
            "В ответе place — НОВОЕ состояние партии: board, current, next, score, level, "
            "game_over, win, features (см. правило 10) и lines_cleared (стёрто линий за пакет), "
            "score_delta (прирост счёта). "
            "4) Невозможные ходы (стена, другая фигура) игнорируются: фигура остаётся на месте, "
            "повторять ход не нужно. "
            "5) Фиксация — автоматическая после пакета place (игра опускает фигуру до упора). "
            "6) Счёт: 1 линия = 1, 2 = 3, 3 = 6, 4 = 10 (умножается на уровень); "
            "уровень растёт со счётом (пороги 50, 100, 150, ...). "
            "7) Game over — если стек дошёл до верхних трёх рядов (y 0..2). "
            "8) Победа — 25-й уровень (счёт 8000). "
            "9) КОНЕЦ ПАРТИИ (game_over: true ИЛИ win: true) — это ТЕРМИНАЛЬНОЕ состояние: "
            "партия окончена, ходить НЕЛЬЗЯ (place и одиночные ходы будут отклонены). "
            "Единственный доступный ход — restart (новая партия) или quit (выйти из игры). "
            "Как только в place или get_state видите game_over: true или win: true — "
            "немедленно остановитесь и вызовите restart; НЕ продолжайте ходить. "
            "10) features — признаки поля (не нужно вручную пересчитывать board): "
            "heights — высоты колонок (0 — пустая, 21 — заполнена до верхнего ряда), "
            "holes — закрытые пустые клетки (над ними в той же колонке есть блок), "
            "bumpiness — суммарная неровность поверхности, aggregate_height — общая высота блоков. "
            "Стратегия: выбирайте расстановку, минимизирующую holes + bumpiness + aggregate_height, "
            "избегайте ям и выступов. Сначала оцените ориентацию фигуры (её cells), "
            "затем выбирайте колонку. Повороты: A=I — часто нужна вертикаль (4x1, то есть 1x4) — "
            "поворачивайте перед сдвигом; B=O — квадрат, поворот ничего не меняет, не поворачивайте; "
            "C/D/E/F/G — поворот часто снижает holes + bumpiness. "
            "Следите за next, чтобы готовить место. "
            "11) Одиночные ходы (action): left / right / rotate_up / rotate_down / move_down / "
            "hard_drop — доступны для отладки; для игры используйте place. "
            "Формат get_state / place: board — 22 строки по 10 символов, '.' — пусто, "
            "буква — id зафиксированной фигуры (A..G); current — текущая фигура: "
            "id (A..G), cells — её клетки в координатах поля [x, y], min_x/max_x — "
            "левый/правый край (колонка), min_y/max_y — верхний/нижний край (ряд); "
            "next — следующая фигура (то же: id, cells, границы — для планирования); "
            "score, level, game_over, win, features (см. правило 10). "
            "Важно: фигура появляется в СЛУЧАЙНОМ повороте — «стандартную» ориентацию "
            "допускать нельзя, ориентация видна только из cells (и границ). "
            "У левых/правых краев поля left/right могут работать нелинейно "
            "(сдвиг меньше колонки, «кик»): не считайте сдвиги, всегда сверяйте "
            "позицию с current в ответе. "
            "Символы board: A=I (4x1), B=O (2x2), C=T, D=S, E=Z, F=J, G=L. "
            "Перед началом партии и при сомнении вызовите get_rules.";
    }

    // Имя действия протокола (поле "action" команды action, элементы
    // пакета place) → PlayerAction. false — неизвестное имя.
    static bool MapActionName(const std::string &name, PlayerAction &playerAction)
    {
        if (name == "left")
        {
            playerAction = PlayerAction::Left;
            return true;
        }

        if (name == "right")
        {
            playerAction = PlayerAction::Right;
            return true;
        }

        if (name == "rotate_up")
        {
            playerAction = PlayerAction::RotateUp;
            return true;
        }

        if (name == "rotate_down")
        {
            playerAction = PlayerAction::RotateDown;
            return true;
        }

        if (name == "move_down")
        {
            playerAction = PlayerAction::Down;
            return true;
        }

        if (name == "hard_drop")
        {
            playerAction = PlayerAction::HardDrop;
            return true;
        }

        return false;
    }

    // Клетки фигуры в координатах поля: [x, y], x в 0..9, y в 0..21.
    // Фигура описывается 16-битной маской (4x4, бит 15 — левый верхний угол);
    // обход идентичен Game.cpp / GameField.cpp: x += 2, shift >>= 1.
    static JsonValue FigureCells(const FigureSnapshot &figure)
    {
        JsonValue cells;
        cells.type = JsonValue::Type::Array;

        if (figure.id < 0 || figure.id >= static_cast<int>(FIGURES.size()))
        {
            return cells;
        }

        const auto &masks = FIGURES[static_cast<std::size_t>(figure.id)];

        if (figure.position < 0 || figure.position >= static_cast<int>(masks.size()))
        {
            return cells;
        }

        const unsigned short mask = masks[static_cast<std::size_t>(figure.position)];
        const int x0 = figure.x - CURRENT_X1;
        // Обход маски идентичен Game.cpp / GameField.cpp: один бегущий shift
        // (1 << 15) на все 16 бит, x — по колонкам стека (шаг SHIFT_MULTIPLIER).
        unsigned short shift = 1 << 15;

        for (int y = 0; y < SIDE_SQUARE; ++y)
        {
            for (int x = 0; x < SIDE_SQUARE * SHIFT_MULTIPLIER; x += 2, shift >>= 1)
            {
                if (mask & shift)
                {
                    JsonValue cell;
                    cell.type = JsonValue::Type::Array;
                    // x0 + x — колонка стека (логическая клетка = 2 колонки);
                    // координата в поле = колонка стека / SHIFT_MULTIPLIER.
                    cell.arrayValue.push_back(JsonValue((x0 + x) / SHIFT_MULTIPLIER));
                    cell.arrayValue.push_back(JsonValue(figure.y + y));
                    cells.arrayValue.push_back(cell);
                }
            }
        }

        return cells;
    }

    // Границы фигуры в координатах поля (min_x/max_x — левый/правый край в
    // колонках, min_y/max_y — верхний/нижний край в рядах): LLM-агенту не
    // нужно вручную выводить края из cells — смещение фигуры внутри её
    // 4x4-маски уже учтено. Считается той же обходкой, что FigureCells.
    static void AddFigureBounds(JsonValue &figure, const FigureSnapshot &figureSnap)
    {
        if (figureSnap.id < 0 || figureSnap.id >= static_cast<int>(FIGURES.size()))
        {
            return;
        }

        const auto &masks = FIGURES[static_cast<std::size_t>(figureSnap.id)];

        if (figureSnap.position < 0 || figureSnap.position >= static_cast<int>(masks.size()))
        {
            return;
        }

        const unsigned short mask = masks[static_cast<std::size_t>(figureSnap.position)];
        const int x0 = figureSnap.x - CURRENT_X1;
        int minX = 9;
        int maxX = -1;
        int minY = 99;
        int maxY = -1;
        unsigned short shift = 1 << 15;

        for (int y = 0; y < SIDE_SQUARE; ++y)
        {
            for (int x = 0; x < SIDE_SQUARE * SHIFT_MULTIPLIER; x += 2, shift >>= 1)
            {
                if (mask & shift)
                {
                    const int fieldX = (x0 + x) / SHIFT_MULTIPLIER;
                    const int fieldY = figureSnap.y + y;

                    if (fieldX < minX)
                    {
                        minX = fieldX;
                    }

                    if (fieldX > maxX)
                    {
                        maxX = fieldX;
                    }

                    if (fieldY < minY)
                    {
                        minY = fieldY;
                    }

                    if (fieldY > maxY)
                    {
                        maxY = fieldY;
                    }
                }
            }
        }

        if (maxX >= 0)
        {
            figure["min_x"] = JsonValue(minX);
            figure["max_x"] = JsonValue(maxX);
            figure["min_y"] = JsonValue(minY);
            figure["max_y"] = JsonValue(maxY);
        }
    }

    // Заполняет JsonValue полями состояния. Поле — человекочитаемой сеткой
    // (22 строки по 10 символов), фигуры — клетками в координатах поля:
    // LLM-агенту не нужно разбирать внутренний формат стека (21 колонка,
    // где логическая клетка = 2 колонки).
    void AgentChannel::FillState(JsonValue &root, const GameStateSnapshot &state)
    {
        root["score"] = static_cast<int64_t>(state.score);
        root["level"] = state.level;
        root["game_over"] = state.gameOver;
        root["win"] = state.win;

        // Стек: 22 ряда, WIDTH+1 = 21 колонка (последняя — счётчик строки).
        // Логическая клетка = 2 колонки стека (SHIFT_MULTIPLIER).
        JsonValue board;
        board.type = JsonValue::Type::Array;

        for (const auto &row : state.stack)
        {
            std::string line;
            line.reserve(static_cast<std::size_t>(WIDTH / SHIFT_MULTIPLIER));

            for (int x = 0; x < WIDTH; x += SHIFT_MULTIPLIER)
            {
                const int id = row[static_cast<std::size_t>(x)].first > 0
                    ? row[static_cast<std::size_t>(x)].second
                    : -1;
                line += (id >= 0 && id < 26) ? static_cast<char>('A' + id) : '.';
            }

            board.arrayValue.push_back(JsonValue(line));
        }

        root["board"] = board;
        FillFeatures(root, state);

        JsonValue current;
        current["id"] = state.currentFigure.inUse ? state.currentFigure.id : -1;
        current["cells"] = FigureCells(state.currentFigure);
        AddFigureBounds(current, state.currentFigure);
        root["current"] = current;

        JsonValue next;
        next["id"] = state.nextFigure.id;
        next["cells"] = FigureCells(state.nextFigure);
        AddFigureBounds(next, state.nextFigure);
        root["next"] = next;
    }

    // Признаки поля для LLM-агента (совет «готовые признаки»): агенту не
    // нужно вручную считать ряды board — готовое сравнение расстановок:
    // heights — высоты колонок (0 — пустая колонка, 21 — заполнена до
    // верхнего ряда), holes — закрытые пустые клетки (над ними в той же
    // колонке есть блок), bumpiness — суммарная неровность поверхности,
    // aggregate_height — общая высота блоков. Цель: минимизировать
    // holes + bumpiness + aggregate_height. Считаются из стека — движок
    // не затрагивается.
    void AgentChannel::FillFeatures(JsonValue &root, const GameStateSnapshot &state)
    {
        int heights[10] = { 0 };

        for (int col = 0; col < WIDTH / SHIFT_MULTIPLIER; ++col)
        {
            for (int row = 0; row < static_cast<int>(state.stack.size()); ++row)
            {
                const auto &rowData = state.stack[row];

                if (rowData[static_cast<std::size_t>(col * SHIFT_MULTIPLIER)].first > 0)
                {
                    heights[col] = row;
                    break;
                }
            }
        }

        int holes = 0;
        int aggregateHeight = 0;

        for (int col = 0; col < WIDTH / SHIFT_MULTIPLIER; ++col)
        {
            aggregateHeight += HEIGHT - heights[col];

            for (int row = 0; row < heights[col]; ++row)
            {
                const auto &rowData = state.stack[row];

                if (rowData[static_cast<std::size_t>(col * SHIFT_MULTIPLIER)].first <= 0)
                {
                    ++holes;
                }
            }
        }

        int bumpiness = 0;

        for (int col = 0; col + 1 < WIDTH / SHIFT_MULTIPLIER; ++col)
        {
            const int diff = heights[col] - heights[col + 1];
            bumpiness += diff < 0 ? -diff : diff;
        }

        JsonValue features;
        features["aggregate_height"] = aggregateHeight;

        JsonValue heightsArray;
        heightsArray.type = JsonValue::Type::Array;

        for (int col = 0; col < WIDTH / SHIFT_MULTIPLIER; ++col)
        {
            heightsArray.arrayValue.push_back(JsonValue(heights[col]));
        }

        features["heights"] = heightsArray;
        features["holes"] = holes;
        features["bumpiness"] = bumpiness;
        root["features"] = features;
    }

    // Обслуживает команду place (пакет: полная расстановка фигуры одной
    // командой): валидация, постановка в очередь игрового цикла, ожидание
    // выполнения пакета (счётчик placeGeneration) и новое состояние партии
    // в ответе. Ответ формирует response (id уже записан HandleCommand).
    void AgentChannel::HandlePlace(const JsonValue &request, JsonValue &response)
    {
        if (session == nullptr)
        {
            response["type"] = std::string("error");
            response["error"] = std::string("no session");
            return;
        }

        // Терминальное состояние: партия окончена — только restart/quit.
        const GameStateSnapshot state = session->GetState();

        if (state.gameOver || state.win)
        {
            response["type"] = std::string("error");
            response["error"] = std::string("game over: only restart or quit");
            return;
        }

        const JsonValue *actionsValue = request.Find("actions");

        if (actionsValue == nullptr
            || actionsValue->type != JsonValue::Type::Array
            || actionsValue->arrayValue.empty())
        {
            response["type"] = std::string("error");
            response["error"] = std::string("actions: требуется непустой массив действий");
            return;
        }

        if (actionsValue->arrayValue.size() > kMaxPlaceActions)
        {
            response["type"] = std::string("error");
            response["error"] = std::string("actions: максимум " + std::to_string(kMaxPlaceActions) + " действий");
            return;
        }

        std::vector<PlayerAction> actions;
        actions.reserve(actionsValue->arrayValue.size());

        for (const JsonValue &actionValue : actionsValue->arrayValue)
        {
            const std::string actionName = actionValue.AsString();
            PlayerAction playerAction{};

            if (!MapActionName(actionName, playerAction))
            {
                response["type"] = std::string("error");
                response["error"] = std::string("unknown action: " + actionName);
                return;
            }

            actions.push_back(playerAction);
        }

        // Ждём завершения пакета: игровой цикл инкрементирует placeGeneration
        // после очистки линий и выдачи новой фигуры. Пакет всегда завершает
        // раунд (авто-hard_drop), поэтому потерянный пакет невозможен.
        const unsigned long long generation = session->GetPlaceGeneration();
        session->EnqueuePlace(std::move(actions));

        const DWORD64 deadline = GetTickCount64() + kPlaceWaitTimeoutMs;
        bool done = false;

        while (!done && !stopRequested.load())
        {
            if (session->GetPlaceGeneration() != generation)
            {
                done = true;
            }
            else
            {
                Sleep(kPlaceWaitPollMs);

                if (GetTickCount64() >= deadline)
                {
                    break;
                }
            }
        }

        if (!done)
        {
            response["type"] = std::string("error");
            response["error"] = std::string("place: таймаут ожидания выполнения пакета");
            return;
        }

        const AgentPlaceResult placeResult = session->GetPlaceResult();
        response["type"] = std::string("place");
        response["lines_cleared"] = placeResult.linesCleared;
        response["score_delta"] = static_cast<int64_t>(placeResult.scoreDelta);
        // Полное новое состояние партии: board, features, current, next,
        // score, level, game_over, win.
        FillState(response, session->GetState());
    }

    // Краткое описание запроса для лога: для action — ход, для place —
    // список ходов пакета (виден сам «план» агента), для остальных — пусто.
    static std::string RequestSummary(const JsonValue &request)
    {
        const JsonValue *actionValue = request.Find("action");

        if (actionValue != nullptr)
        {
            return "action=" + actionValue->AsString();
        }

        const JsonValue *actionsValue = request.Find("actions");

        if (actionsValue == nullptr || actionsValue->type != JsonValue::Type::Array)
        {
            return std::string();
        }

        std::string summary;
        bool first = true;

        for (const JsonValue &actionValue : actionsValue->arrayValue)
        {
            if (!first)
            {
                summary += ',';
            }

            first = false;
            summary += actionValue.AsString();
        }

        if (summary.empty())
        {
            return std::string();
        }

        return "actions=" + summary;
    }

    // Краткое сводное состояние ответа для лога: score/level/holes/bump
    // для состояний партии, lines/delta для place, game_over/win при
    // конце партии, error — текст ошибки (для ok/rules — пусто).
    static std::string ResponseSummary(const JsonValue &response)
    {
        const JsonValue *errorValue = response.Find("error");

        if (errorValue != nullptr)
        {
            return "error=" + errorValue->AsString();
        }

        const JsonValue *scoreValue = response.Find("score");

        if (scoreValue == nullptr)
        {
            return std::string();
        }

        std::string summary = "score=" + std::to_string(scoreValue->AsInt())
            + " level=" + std::to_string(response.Find("level")->AsInt());

        const JsonValue *featuresValue = response.Find("features");

        if (featuresValue != nullptr)
        {
            const JsonValue *holesValue = featuresValue->Find("holes");

            if (holesValue != nullptr)
            {
                summary += " holes=" + std::to_string(holesValue->AsInt());
            }

            const JsonValue *bumpValue = featuresValue->Find("bumpiness");

            if (bumpValue != nullptr)
            {
                summary += " bump=" + std::to_string(bumpValue->AsInt());
            }
        }

        const JsonValue *linesValue = response.Find("lines_cleared");

        if (linesValue != nullptr)
        {
            summary += " lines=" + std::to_string(linesValue->AsInt());

            const JsonValue *deltaValue = response.Find("score_delta");
            summary += " delta=" + std::to_string(deltaValue != nullptr ? deltaValue->AsInt() : 0);
        }

        const JsonValue *overValue = response.Find("game_over");

        if (overValue != nullptr && overValue->AsBool())
        {
            summary += " game_over";
        }

        const JsonValue *winValue = response.Find("win");

        if (winValue != nullptr && winValue->AsBool())
        {
            summary += " win";
        }

        return summary;
    }

    // Обслуживает одну команду (JSON): формирует ответ, пишет его в лог
    // (если включён флаг --agent-diag) и отправляет клиенту.
    void AgentChannel::HandleCommand(const std::string &line)
    {
        JsonValue request;

        if (!ParseJson(line, request))
        {
            LogEvent(true, "invalid_json", 0, std::string());
            (void)SendLine("{\"type\":\"error\",\"error\":\"invalid json\"}");
            return;
        }

        const JsonValue *idValue = request.Find("id");
        const int64_t id = idValue != nullptr ? idValue->AsInt() : 0;
        const JsonValue *typeValue = request.Find("type");
        const std::string type = typeValue != nullptr ? typeValue->AsString() : std::string();

        LogEvent(true, type, id, RequestSummary(request));

        JsonValue response;
        response["id"] = id;

        if (type == "get_state")
        {
            if (session == nullptr)
            {
                response["type"] = std::string("error");
                response["error"] = std::string("no session");
            }
            else
            {
                response["type"] = std::string("state");
                FillState(response, session->GetState());
            }
        }
        else if (type == "action")
        {
            const JsonValue *actionValue = request.Find("action");
            const std::string action = actionValue != nullptr ? actionValue->AsString() : std::string();

            PlayerAction playerAction{};

            if (MapActionName(action, playerAction) && session != nullptr)
            {
                session->EnqueueAction(playerAction);
                response["type"] = std::string("ok");
            }
            else
            {
                response["type"] = std::string("error");
                response["error"] = std::string(session == nullptr ? "no session" : "unknown action");
            }
        }
        else if (type == "place")
        {
            HandlePlace(request, response);
        }
        else if (type == "get_rules")
        {
            response["type"] = std::string("rules");
            response["rules"] = GetRulesText();
        }
        else if (type == "restart")
        {
            if (session != nullptr)
            {
                session->RequestRestart();
                response["type"] = std::string("ok");
            }
            else
            {
                response["type"] = std::string("error");
                response["error"] = std::string("no session");
            }
        }
        else if (type == "quit")
        {
            quitRequested = true;

            if (session != nullptr)
            {
                session->RequestQuit();
            }

            response["type"] = std::string("ok");
        }
        else
        {
            response["type"] = std::string("error");
            response["error"] = std::string("unknown type");
        }

        LogEvent(false, type, id, ResponseSummary(response));
        (void)SendLine(SerializeJson(response));
    }

    // Запускает сервер: bind/listen/accept. Возвращает порт (или -1 при
    // ошибке). Блокируется, пока не вызван Stop() (агентский поток).
    int AgentChannel::Start()
    {
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (listenSocket == INVALID_SOCKET)
        {
            return -1;
        }

        int port = -1;

        for (int i = 0; i < kPortCount; ++i)
        {
            const int candidate = kFirstPort + i;
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<u_short>(candidate));
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

            if (bind(listenSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0)
            {
                port = candidate;
                boundPort.store(port); // основной поток узнаёт порт до старта игры
                break;
            }
        }

        if (port == -1)
        {
            (void)closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
            return -1;
        }

        if (listen(listenSocket, 1) != 0)
        {
            (void)closesocket(listenSocket);
            listenSocket = INVALID_SOCKET;
            return -1;
        }

        // Цикл accept: обслуживает клиентов последовательно (один за раз).
        // При разрыве соединения ждёт следующего клиента, пока не Stop().
        while (!stopRequested.load())
        {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(listenSocket, &readSet);
            timeval timeout{ 0, 500000 }; // 0.5 с
            const int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);

            if (selectResult < 0)
            {
                break;
            }

            if (selectResult == 0)
            {
                continue; // таймаут — проверяем стоп
            }

            clientSocket = accept(listenSocket, nullptr, nullptr);

            if (clientSocket == INVALID_SOCKET)
            {
                continue;
            }

            while (!stopRequested.load())
            {
                std::string line;

                if (!ReadLine(line))
                {
                    break; // разрыв соединения или стоп
                }

                HandleCommand(line);
            }

            (void)closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }

        // Сокеты закрываем сам поток (безопасно: никто больше в них не висит).
        if (clientSocket != INVALID_SOCKET)
        {
            (void)closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }

        (void)closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;

        return port;
    }

    // Останавливает сервер: ставит флаг, поток завершится сам.
    void AgentChannel::Stop()
    {
        stopRequested.store(true);
    }

    bool AgentChannel::IsQuitRequested() const
    {
        return quitRequested;
    }

    int AgentChannel::GetBoundPort() const
    {
        return boundPort.load();
    }
}
