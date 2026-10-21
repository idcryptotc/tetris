#include "GameData.h"
#include "Figure.h"
#include "Keys.h"
#include "Settings.h"

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <Windows.h>

namespace Tetris
{
    // Всё, что нужно для работы с двумя буферами консоли (двойная буферизация).
    // Единственный экземпляр — g_console; инициализируется в ProgramSettings().
    struct ConsoleRenderer
    {
        HANDLE basicBuffer{ INVALID_HANDLE_VALUE };     // Основной (видимый) буфер
        HANDLE doubleBuffer{ INVALID_HANDLE_VALUE };    // Резервный буфер для отрисовки
        HWND hWnd{ nullptr };                           // Окно консоли
        RECT basicRect{};                               // Размеры окна в момент инициализации
        CONSOLE_SCREEN_BUFFER_INFO csbi{};              // Информация о буфере (используется в DoubleBufferingBegin)
        SMALL_RECT bufferRect{};                        // Область копирования между буферами
        COORD bufferSize{};                             // Размер области копирования
        COORD bufferCoord{};                            // Начальная точка копирования
        CHAR_INFO buffer[2000]{};                       // Область копирования (80x25)
    };

    ConsoleRenderer g_console;

    void MyErrorExit(const wchar_t *comment);

    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level
    );

    void ViewGameField(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField);
    int PositionById(int figureId);
    int StartMenu();
    MenuResult ReturnToMenu();
    void ProgramSettings();
    void DoubleBufferingBegin();
    void DoubleBufferingEnd();
    bool PlayGame();
    int RandomInt(int bound);
    int RandomFigureId();
    bool CheckCollisions(const Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction);
    bool TryMove(Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction, Actions commitAction);
    FallResult TryFallOneStep(Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack, int line);
    void SetFigure(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack);
    bool CheckGameOver(const std::vector<std::vector<std::pair<int, int>>> &gameStack);
    void ClearKeyboardBuffer();

    void ClearLines
    (
        std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const Figure &nextFigure,
        unsigned int &score,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const int &level
    );

    void CheckScoreForLevel(const unsigned int &score, int &level);

    void CreateGameField
    (
        const Figure &nextFigure,
        const Figure &currentFigure,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const unsigned int &score,
        const int &level
    )
    {
        int idFigure = nextFigure.GetId();
        int positionFigure = nextFigure.GetPosition();
        int colorFigureAsInt = nextFigure.GetColorAsInt();
        unsigned short shift = 1 << 15;

        for (int y = NEXT_Y1; y < NEXT_Y2 + 1; ++y)
        {
            for (int x = NEXT_X1; x < NEXT_X2 + 1; x += SHIFT_MULTIPLIER, shift >>= 1)
            {
                if (FIGURES[idFigure][positionFigure] & shift)
                {
                    gameField[y].first[x] = L'█';
                    gameField[y].first[x + 1] = L'█';
                    gameField[y].second[x] = colorFigureAsInt;
                    gameField[y].second[x + 1] = colorFigureAsInt;
                }
                else
                {
                    gameField[y].first[x] = L' ';
                    gameField[y].first[x + 1] = L' ';
                }
            }
        }

        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                gameField[y].first[CURRENT_X1 + x] = gameStack[y][x].first ? L'█' : L' ';
                gameField[y].second[CURRENT_X1 + x] = gameStack[y][x].second;
            }
        }

        int currentScore = score;

        for (int i = 0; i < 6; ++i)
        {
            gameField[SCORE_Y].first[SCORE_X - i] = (wchar_t)(currentScore % 10 + L'0');
            currentScore /= 10;
        }

        int currentLevel = level;

        for (int i = 0; i < 2; ++i)
        {
            gameField[LEVEL_Y].first[LEVEL_X - i] = (wchar_t)(currentLevel % 10 + L'0');
            currentLevel /= 10;
        }

        if (currentFigure.GetUse())
        {
            idFigure = currentFigure.GetId();
            positionFigure = currentFigure.GetPosition();
            colorFigureAsInt = currentFigure.GetColorAsInt();
            int currentFigurePositionYStart = currentFigure.GetPositionY();
            int currentFigurePositionYFinish = currentFigure.GetPositionY() + SIDE_SQUARE;
            int currentFigurePositionXStart = currentFigure.GetPositionX();
            int currentFigurePositionXFinish = currentFigure.GetPositionX() + SIDE_SQUARE * SHIFT_MULTIPLIER;
            shift = 1 << 15;

            for (int y = currentFigurePositionYStart; y < currentFigurePositionYFinish; ++y)
            {
                for (int x = currentFigurePositionXStart; x < currentFigurePositionXFinish; x += 2, shift >>= 1)
                {
                    if (FIGURES[idFigure][positionFigure] & shift)
                    {
                        gameField[y].first[x] = L'█';
                        gameField[y].first[x + 1] = L'█';
                        gameField[y].second[x] = colorFigureAsInt;
                        gameField[y].second[x + 1] = colorFigureAsInt;
                    }
                }
            }
        }

        ViewGameField(gameField);
    }

    void ViewGameField(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField)
    {
        DoubleBufferingBegin();

        for (const auto &i : gameField)
        {
            std::size_t iFirstSize = i.first.size();

            for (std::size_t j = 0; j < iFirstSize; ++j)
            {
                if (i.first[j] == L'█')
                {
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(i.second[j]));
                    std::wcout << i.first[j];
                    SetConsoleTextAttribute(g_console.basicBuffer, (WORD)(Colors::LIGHTGRAY));
                }
                else
                {
                    std::wcout << i.first[j];
                }
            }

            std::wcout << std::endl;
        }

        DoubleBufferingEnd();
    }

    // Случайное целое в диапазоне [0, bound).
    int RandomInt(int bound)
    {
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution<int> dist(0, bound - 1);
        return dist(rng);
    }

    int RandomFigureId()
    {
        return RandomInt(static_cast<int>(FIGURES.size()));
    }

    int PositionById(int figureId)
    {
        switch (figureId)
        {
            case 0:case 1:case 2:
                {
                    return RandomInt(10);
                }
            case 3:case 4:case 5:
                {
                    return RandomInt(5);
                }
            case 6:
                {
                    return RandomInt(3);
                }
            default:
                {
                    return -1;
                }
        }
    }

    int StartMenu()
    {
        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Настройка клавиш" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;
            menuItem = _getch() - '0';

            if (menuItem < 1 || menuItem > 3)
            {
                system("cls");
                continue;
            }

            system("cls");
            return menuItem;
        }
    }

    MenuResult ReturnToMenu()
    {
        system("cls");
        ClearKeyboardBuffer();
        int menuItem;

        while (true)
        {
            std::wcout << L"Меню" << std::endl;
            std::wcout << L"1 - Новая игра" << std::endl;
            std::wcout << L"2 - Продолжить" << std::endl;
            std::wcout << L"3 - Выход" << std::endl;
            menuItem = _getch() - '0';

            if (menuItem < 1 || menuItem > 3)
            {
                system("cls");
            }
            else
            {
                break;
            }
        }

        return static_cast<MenuResult>(menuItem);
    }

    void MyErrorExit(const wchar_t *comment)
    {
        system("cls");
        std::wcout << comment;
        (void)_getch(); // ждём нажатия, символ не нужен
        exit(0);
    }

    void ProgramSettings()
    {
        (void)_setmode(_fileno(stdout), _O_U16TEXT);
        srand(static_cast<unsigned>(time(NULL)));
        system("mode con cols=80 lines=25");
        g_console.hWnd = GetConsoleWindow();
        GetWindowRect(g_console.hWnd, &g_console.basicRect);
        g_console.basicBuffer = GetStdHandle(STD_OUTPUT_HANDLE);

        g_console.doubleBuffer = CreateConsoleScreenBuffer
        (
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CONSOLE_TEXTMODE_BUFFER,
            NULL
        );

        if (g_console.basicBuffer == INVALID_HANDLE_VALUE ||
            g_console.doubleBuffer == INVALID_HANDLE_VALUE)
        {
            MyErrorExit(L"CreateConsoleScreenBuffer");
            return;
        }

        CONSOLE_FONT_INFOEX fontInfo{};
        fontInfo.cbSize = sizeof(fontInfo);

        GetCurrentConsoleFontEx(g_console.basicBuffer, TRUE, &fontInfo);
        fontInfo.dwFontSize.Y = 24;
        SetCurrentConsoleFontEx(g_console.basicBuffer, TRUE, &fontInfo);

        GetCurrentConsoleFontEx(g_console.doubleBuffer, TRUE, &fontInfo);
        fontInfo.dwFontSize.Y = 24;
        SetCurrentConsoleFontEx(g_console.doubleBuffer, TRUE, &fontInfo);

        CONSOLE_CURSOR_INFO structCursorInfo;

        GetConsoleCursorInfo(g_console.doubleBuffer, &structCursorInfo);
        structCursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(g_console.doubleBuffer, &structCursorInfo);

        GetConsoleCursorInfo(g_console.basicBuffer, &structCursorInfo);
        structCursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(g_console.basicBuffer, &structCursorInfo);

        system("mode con cols=80 lines=25");
        g_console.hWnd = GetConsoleWindow();
        GetWindowRect(g_console.hWnd, &g_console.basicRect);

        if (!SetConsoleActiveScreenBuffer(g_console.doubleBuffer))
        {
            MyErrorExit(L"SetConsoleActiveScreenBuffer");
        }

        g_console.bufferSize.Y = 25;
        g_console.bufferSize.X = 80;

        g_console.bufferCoord.X = 0;
        g_console.bufferCoord.Y = 0;

        g_console.bufferRect.Top = 0;
        g_console.bufferRect.Left = 0;
        g_console.bufferRect.Bottom = 24;
        g_console.bufferRect.Right = 79;

        BOOL fSuccess = ReadConsoleOutput(
            g_console.basicBuffer,
            g_console.buffer,
            g_console.bufferSize,
            g_console.bufferCoord,
            &g_console.bufferRect);

        if (!fSuccess)
        {
            MyErrorExit(L"ReadConsoleOutput");
        }


        fSuccess = WriteConsoleOutput(
            g_console.doubleBuffer,
            g_console.buffer,
            g_console.bufferSize,
            g_console.bufferCoord,
            &g_console.bufferRect);

        if (!fSuccess)
        {
            MyErrorExit(L"WriteConsoleOutput");
        }

        if (!SetConsoleActiveScreenBuffer(g_console.basicBuffer))
        {
            MyErrorExit(L"SetConsoleActiveScreenBuffer");
        }
    }

    void DoubleBufferingBegin()
    {
        RECT currentRect;
        GetWindowRect(g_console.hWnd, &currentRect);

        if (currentRect.right - currentRect.left != g_console.basicRect.right - g_console.basicRect.left
            || currentRect.bottom - currentRect.top != g_console.basicRect.bottom - g_console.basicRect.top)
        {
            SetConsoleScreenBufferSize(g_console.basicBuffer, g_console.csbi.dwSize);
            SetConsoleScreenBufferSize(g_console.doubleBuffer, g_console.csbi.dwSize);
            system("mode con cols=80 lines=25");
        }

        BOOL fSuccess = ReadConsoleOutput(
            g_console.basicBuffer,
            g_console.buffer,
            g_console.bufferSize,
            g_console.bufferCoord,
            &g_console.bufferRect);

        if (!fSuccess)
        {
            MyErrorExit(L"ReadConsoleOutput");
        }

        fSuccess = WriteConsoleOutput(
            g_console.doubleBuffer,
            g_console.buffer,
            g_console.bufferSize,
            g_console.bufferCoord,
            &g_console.bufferRect);

        if (!fSuccess)
        {
            MyErrorExit(L"WriteConsoleOutput");
        }

        SetConsoleActiveScreenBuffer(g_console.doubleBuffer);

        if (currentRect.right - currentRect.left != g_console.basicRect.right - g_console.basicRect.left
            || currentRect.bottom - currentRect.top != g_console.basicRect.bottom - g_console.basicRect.top)
        {
            system("mode con cols=80 lines=25");
        }
    }

    void DoubleBufferingEnd()
    {
        SetConsoleActiveScreenBuffer(g_console.basicBuffer);
    }

    // Проверяет, не пересекается ли «пробная» позиция фигуры со стеком.
    // Стек не копируется и не изменяется: тетрамино не пересекает само себя,
    // поэтому достаточно проверить, занята ли уже любая из его ячеек.
    bool CheckCollisions(const Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction)
    {
        bool isXNext = direction == MoveDirection::Left
            || direction == MoveDirection::Right;

        bool isYNext = direction == MoveDirection::Drop
            && currentFigure.GetPositionY(true) != static_cast<int>(gameStack.size()) - 3;

        bool isPositionNext = direction != MoveDirection::Drop;
        int currentFigurePositionYStart = currentFigure.GetPositionY(isYNext);
        int currentFigurePositionYFinish = currentFigure.GetPositionY(isYNext) + SIDE_SQUARE;
        int currentFigurePositionXStart = currentFigure.GetPositionX(isXNext) - CURRENT_X1;
        int currentFigurePositionXFinish = currentFigure.GetPositionX(isXNext) - CURRENT_X1 + SIDE_SQUARE * SHIFT_MULTIPLIER;
        int currentFigureId = currentFigure.GetId();
        int currentFigurePosition = currentFigure.GetPosition(isPositionNext);
        unsigned short shift = 1 << 15;

        for (int y = currentFigurePositionYStart; y < currentFigurePositionYFinish; ++y)
        {
            for (int x = currentFigurePositionXStart;
                x < currentFigurePositionXFinish;
                x += 2, shift >>= 1)
            {
                if (FIGURES[currentFigureId][currentFigurePosition] & shift)
                {
                    if (gameStack[y][x].first >= 1 || gameStack[y][x + 1].first >= 1)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    // Делает «пробное» движение фигуры, проверяет столкновение
    // и либо подтверждает его (commitAction), либо откатывает (DEFAULT).
    bool TryMove(Figure &currentFigure, const std::vector<std::vector<std::pair<int, int>>> &gameStack, MoveDirection direction, Actions commitAction)
    {
        switch (direction)
        {
            case MoveDirection::Left:
                currentFigure.ShiftLeft();
                break;
            case MoveDirection::Right:
                currentFigure.ShiftRight();
                break;
            case MoveDirection::RotateUp:
                currentFigure.RotateUp();
                break;
            case MoveDirection::RotateDown:
                currentFigure.RotateDown();
                break;
            case MoveDirection::Drop:
                currentFigure.ShiftDown();
                break;
        }

        if (CheckCollisions(currentFigure, gameStack, direction))
        {
            currentFigure.Action(Actions::DEFAULT); // откат пробного движения
            return false;
        }

        currentFigure.Action(commitAction);
        return true;
    }

    // Сдвигает фигуру на ряд вниз. Locked — фигура зафиксирована в стеке.
    FallResult TryFallOneStep(Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack, int line)
    {
        if (line >= MAX_FALL_LINES)
        {
            return FallResult::Locked;
        }

        currentFigure.ShiftDown();

        if (CheckCollisions(currentFigure, gameStack, MoveDirection::Drop))
        {
            SetFigure(currentFigure, gameStack);
            return FallResult::Locked;
        }

        currentFigure.Action(Actions::DOWN);

        if (line == LAST_FALL_LINE)
        {
            SetFigure(currentFigure, gameStack);
            currentFigure.SetUse(false);
        }

        return FallResult::Fell;
    }

    void SetFigure(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack)
    {
        int lastLine = (currentFigure.GetPositionY() + 3) == gameStack.size() ? 1 : 0;
        int positionYStart = currentFigure.GetPositionY() - lastLine;
        int positionYFinish = currentFigure.GetPositionY() + SIDE_SQUARE - lastLine;
        int positionXStart = currentFigure.GetPositionX() - CURRENT_X1;
        int positionXFinish = currentFigure.GetPositionX() - CURRENT_X1 + SIDE_SQUARE * SHIFT_MULTIPLIER;
        int currentFigureId = currentFigure.GetId();
        int currentFigurePosition = currentFigure.GetPosition();
        int currentFigureColorAsInt = currentFigure.GetColorAsInt();
        unsigned short shift = 1 << 15;

        for (int y = positionYStart; y < positionYFinish; ++y)
        {
            for (int x = positionXStart; x < positionXFinish; x += 2, shift >>= 1)
            {
                if (FIGURES[currentFigureId][currentFigurePosition] & shift)
                {
                    ++gameStack[y][x].first;
                    ++gameStack[y][x + 1].first;
                    gameStack[y][x].second = currentFigureColorAsInt;
                    gameStack[y][x + 1].second = currentFigureColorAsInt;
                    ++gameStack[y][WIDTH].first;
                }
            }
        }
    }

    bool PlayGame()
    {
        int levelGame = 1;
        unsigned int score = 0;
        int stepProgress;
        int figureId = RandomFigureId();
        int figurePosition = PositionById(figureId);
        Figure nextFigure(figureId, figurePosition);
        figureId = RandomFigureId();
        figurePosition = PositionById(figureId);
        Figure currentFigure;
        std::vector<std::vector<std::pair<int, int>>> gameStack(HEIGHT, std::vector<std::pair<int, int>>(WIDTH + 1, { 0,0 }));
        std::vector<std::pair<std::wstring, std::vector<int>>> gameField = GAME_FIELD;

        while (true)
        {
            ClearKeyboardBuffer();
            ClearLines(gameStack, nextFigure, score, gameField, levelGame);
            CheckScoreForLevel(score, levelGame);
            currentFigure = nextFigure;
            currentFigure.SetUse(true);
            figureId = RandomFigureId();
            figurePosition = PositionById(figureId);
            nextFigure.SetBasicParameters(figureId, figurePosition);
            CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);

            if (CheckGameOver(gameStack))
            {
                return true;
            }

            for (int line = 0; line < MAX_FALL_LINES; ++line)
            {
                stepProgress = 0;

                while (stepProgress < COUNT_STEP_CHAPTER)
                {
                    // Приоритет клавиш сохранён как в исходной версии:
                    // LEFT > RIGHT > ROTATE_UP > ROTATE_DOWN > HARD_DROP > EXIT.
                    if (IsKeyPressed(KeyFor(BindingAction::LEFT)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::Left, Actions::LEFT);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::RIGHT)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::Right, Actions::RIGHT);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::ROTATE_UP)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::RotateUp, Actions::ROTATE);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::ROTATE_DOWN)))
                    {
                        TryMove(currentFigure, gameStack, MoveDirection::RotateDown, Actions::ROTATE);
                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::HARD_DROP)))
                    {
                        if (line < MAX_FALL_LINES)
                        {
                            if (TryFallOneStep(currentFigure, gameStack, line) == FallResult::Locked)
                            {
                                line = MAX_FALL_LINES;
                                break;
                            }

                            ++line;
                            ++stepProgress;
                            CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                        }
                    }
                    else if (IsKeyPressed(KeyFor(BindingAction::EXIT)))
                    {
                        switch (ReturnToMenu())
                        {
                            case MenuResult::Restart:
                                return true;
                            case MenuResult::Stay:
                                CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                                break;
                            case MenuResult::Quit:
                                return false;
                        }
                    }

                    Sleep(LEVEL_TIME_STEP[levelGame] / COUNT_STEP_CHAPTER);
                    ++stepProgress;
                }

                if (line < MAX_FALL_LINES)
                {
                    if (TryFallOneStep(currentFigure, gameStack, line) == FallResult::Locked)
                    {
                        line = MAX_FALL_LINES;
                        break;
                    }
                }

                CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
            }

            nextFigure.SetUse(false);
        }
    }

    void ClearKeyboardBuffer()
    {
        while (_kbhit())
        {
            (void)_getch(); // чистим буфер, символ не нужен
        }
    }

    bool CheckGameOver(const std::vector<std::vector<std::pair<int, int>>> &gameStack)
    {
        for (std::size_t i = 0; i < 3; ++i)
        {
            for (const auto &j : gameStack[i])
            {
                if (j.first > 0)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void ClearLines
    (
        std::vector<std::vector<std::pair<int, int>>> &gameStack,
        const Figure &nextFigure,
        unsigned int &score,
        std::vector<std::pair<std::wstring, std::vector<int>>> &gameField,
        const int &level
    )
    {
        std::size_t gameStackSize = gameStack.size();

        for (std::size_t i = 3; i < gameStackSize; ++i)
        {
            std::size_t gameStackISize = gameStack[i].size() - 1;

            if (gameStack[i][WIDTH].first == 10)
            {
                unsigned int mask = 0x8;
                gameStack[i][WIDTH].first = -1;

                if (i + 1 < 22)
                {
                    mask += gameStack[i + 1][WIDTH].first == 10 ? (gameStack[i + 1][WIDTH].first = -1, 0x4) : 0x0;
                }

                if (i + 2 < 22)
                {
                    mask += gameStack[i + 2][WIDTH].first == 10 ? (gameStack[i + 2][WIDTH].first = -1, 0x2) : 0x0;
                }

                if (i + 3 < 22)
                {
                    mask += gameStack[i + 3][WIDTH].first == 10 ? (gameStack[i + 3][WIDTH].first = -1, 0x1) : 0x0;
                }

                for (std::size_t j = 0; j < gameStackISize; ++j)
                {
                    mask & 8 ? gameField[i].first[CURRENT_X1 + j] = L' ' : L'i';
                    mask & 4 ? gameField[i + 1].first[CURRENT_X1 + j] = L' ' : L'i';
                    mask & 2 ? gameField[i + 2].first[CURRENT_X1 + j] = L' ' : L'i';
                    mask & 1 ? gameField[i + 3].first[CURRENT_X1 + j] = L' ' : L'i';
                    ViewGameField(gameField);
                }

                // Кумулятивный подсчёт очков:
                // чем больше линий очищено, тем выше базовый счёт:
                // 4 линии = 10 очков, 3 линии = 6, 2 линии = 3, 1 линия = 1;
                // результат умножается на текущий уровень.
                // Таблица: индекс = количество очищенных линий.
                const unsigned int LINES_SCORE_TABLE[5] = { 0, 1, 3, 6, 10 };
                unsigned int clearedLines = ((mask & 0x8) >> 3) + ((mask & 0x4) >> 2) + ((mask & 0x2) >> 1) + (mask & 1);
                score += LINES_SCORE_TABLE[clearedLines] * static_cast<unsigned int>(level);

                for (std::size_t k = gameStackSize - 1, t = k; k > 0 && t > 0; --t)
                {
                    if (gameStack[t][WIDTH].first == -1)
                    {
                        continue;
                    }

                    for (std::size_t j = 0; j < gameStackISize + 1; ++j)
                    {
                        gameStack[k][j] = gameStack[t][j];
                    }

                    --k;
                }

                break;
            }
        }
    }

    void CheckScoreForLevel(const unsigned int &score, int &level)
    {
        std::size_t size = LEVEL_SCORE.size();

        for (std::size_t i = level - 1; i < size; ++i)
        {
            if (score > LEVEL_SCORE[i])
            {
                level = i + 1;
            }
            else
            {
                break;
            }
        }
    }
}

int main()
{
    Tetris::ProgramSettings();

    // Настройки клавиш из tetris_settings.ini
    Tetris::LoadBindings(Tetris::g_bindings);

    while (true)
    {
        system("cls");
        int menuItem = Tetris::StartMenu();

        if (menuItem == 3)
        {
            return 0; // Выход
        }

        if (menuItem == 2)
        {
            Tetris::ConfigureBindings(Tetris::g_bindings);
            continue; // Вернуться в меню, а не сразу в игру
        }

        if (!Tetris::PlayGame())
        {
            break; // Выбранный в игре «Выход»
        }

        // PlayGame() вернул true — конец партии, предлагаем сыграть снова
    }

    return 0;
}
