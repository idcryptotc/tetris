#include "stdafx.h"

#include "Keys.h"

// Точечный импорт: тянем в глобальное пространство только то, что используем,
// а не весь namespace Tetris (как это делала бы using-директива).
using Tetris::KeyCodes;
using Tetris::IsKeyPressed;
using Tetris::KeyBindings;
using Tetris::kDefaultBindings;

enum class Actions
{
    LEFT, RIGHT, DOWN, ROTATE, DEFAULT
};

enum class Colors
{
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

const int SHIFT_MULTIPLIER = 2;
const int SIDE_SQUARE = 4;
const int WIDTH = 10 * SHIFT_MULTIPLIER, HEIGHT = 22;
const int BASIC_X = 12, BASIC_Y = 0;
const std::vector<unsigned int> LEVEL_TIME_STEP
{
    500,480,460,440,420,
    400,380,360,340,320,
    300,280,260,240,220,
    200,180,160,140,120,
    100,80,60,40,20
};
const std::vector<unsigned int> LEVEL_SCORE
{
    0,50,100,150,200,
    300,400,500,600,700,
    900,1100,1300,1500,1700,
    2000,2300,2600,2900,3200,
    4000,5000,6000,7000,8000
};
const int COUNT_STEP_CHAPTER = 8;
const int NEXT_X1 = 31, NEXT_X2 = 38, NEXT_Y1 = 1, NEXT_Y2 = 4;
const int CURRENT_X1 = 6, CURRENT_X2 = 25, CURRENT_Y1 = 3, CURRENT_Y2 = 21;
const int SCORE_X = 40, SCORE_Y = 7;
const int LEVEL_X = 40, LEVEL_Y = 9;

const std::vector<std::vector<unsigned short>> FIGURES =
{
    {0x088c,0x0446,0x0223,0x002e,0x0017,0x0c44,0x0622,0x0311,0x00e8,0x0074},
    {0x044c,0x0226,0x0113,0x008e,0x0047,0x0c88,0x0644,0x0322,0x00e2,0x0071},
    {0x08c8,0x0464,0x0232,0x004e,0x0027,0x04c4,0x0262,0x0131,0x00e4,0x0072},
    {0x08c4,0x0462,0x0231,0x006c,0x0036},
    {0x04c8,0x0264,0x0132,0x00c6,0x0063},
    {0x8888,0x4444,0x2222,0x1111,0x000f},
    {0x00cc,0x0066,0x0033}
};

const std::map<int, Colors> FIGURE_COLORS
{
    {0,Colors::CYAN},
    {1,Colors::LIGHTBLUE},
    {2,Colors::LIGHTGREEN},
    {3,Colors::MAGENTA},
    {4,Colors::LIGHTRED},
    {5,Colors::YELLOW},
    {6,Colors::DARKGRAY}
};

const std::vector<std::map<int, int>> NEXT_POSITION_LEFT
{
    {{1,0},{2,1},{4,3},{6,5},{7,6},{9,8}},
    {{1,0},{2,1},{4,3},{6,5},{7,6},{9,8}},
    {{1,0},{2,1},{4,3},{6,5},{7,6},{9,8}},
    {{1,0},{2,1},{4,3}},
    {{1,0},{2,1},{4,3}},
    {{1,0},{2,1},{3,2}},
    {{1,0},{2,1}}
};

const std::vector<std::map<int, int>> NEXT_POSITION_RIGHT
{
    {{0,1},{1,2},{3,4},{5,6},{6,7},{8,9}},
    {{0,1},{1,2},{3,4},{5,6},{6,7},{8,9}},
    {{0,1},{1,2},{3,4},{5,6},{6,7},{8,9}},
    {{0,1},{1,2},{3,4}},
    {{0,1},{1,2},{3,4}},
    {{0,1},{1,2},{2,3}},
    {{0,1},{1,2}}
};

const std::vector<std::map<int, int>> NEXT_POSITION_ROTATE_UP
{
    {{0,3},{1,3},{2,4},{3,5},{4,7},{5,8},{6,8},{7,9},{8,0},{9,2}},
    {{0,3},{1,3},{2,4},{3,5},{4,7},{5,8},{6,8},{7,9},{8,0},{9,2}},
    {{0,3},{1,3},{2,4},{3,5},{4,7},{5,8},{6,8},{7,9},{8,0},{9,2}},
    {{0,3},{1,3},{2,4},{3,0},{4,2}},
    {{0,3},{1,3},{2,4},{3,0},{4,2}},
    {{0,4},{1,4},{2,4},{3,4},{4,2}},
    {}
};

const std::vector<std::map<int, int>> NEXT_POSITION_ROTATE_DOWN
{
    {{0,8},{1,8},{2,9},{3,0},{4,2},{5,3},{6,3},{7,4},{8,5},{9,7}},
    {{0,8},{1,8},{2,9},{3,0},{4,2},{5,3},{6,3},{7,4},{8,5},{9,7}},
    {{0,8},{1,8},{2,9},{3,0},{4,2},{5,3},{6,3},{7,4},{8,5},{9,7}},
    {{0,3},{1,3},{2,4},{3,0},{4,2}},
    {{0,3},{1,3},{2,4},{3,0},{4,2}},
    {{0,4},{1,4},{2,4},{3,4},{4,0}},
    {}
};

const std::vector<std::pair<std::wstring, std::vector<int>>> GAME_FIELD
{
    {{L"                              ┌────────┐     "},std::vector<int>(45, 0)},
    {{L"                              │        │     "},std::vector<int>(45, 0)},
    {{L"                              │        │     "},std::vector<int>(45, 0)},
    {{L"     ║                    ║   │        │     "},std::vector<int>(45, 0)},
    {{L"     ║                    ║   │        │     "},std::vector<int>(45, 0)},
    {{L"     ║                    ║   └────────┘     "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║  Score 000000    "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║  Level     01    "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║  ESC - Выход     "},std::vector<int>(45, 0)},
    {{L"     ║                    ║                  "},std::vector<int>(45, 0)},
    {{L"     ║                    ║  ↓ ↑ - Rotate    "},std::vector<int>(45, 0)},
    {{L"     ║                    ║ SPACE - MegaDown "},std::vector<int>(45, 0)},
    {{L"     ║                    ║   ←  - Left      "},std::vector<int>(45, 0)},
    {{L"     ╚════════════════════╝   →  - Right     "},std::vector<int>(45, 0)},
    {{L"                                             "},std::vector<int>(45, 0)}
};

class Figure
{
private:
    int id{};
    Colors color{ Colors::WHITE };
    int position{};
    int nextPosition{ position };
    int x{ BASIC_X };
    int y{ BASIC_Y };
    int nextX{ x };
    int nextY{ y + 1 };
    int multiplier{ SHIFT_MULTIPLIER };
    bool isUse{ true };

public:
    Figure() = default;

    Figure(int id, int p) : id(id), position(p)
    {
        color = FIGURE_COLORS.at(id);
    }

    void SetBasicParameters(int figureId, int figurePosition)
    {
        id = figureId;
        color = FIGURE_COLORS.at(id);
        position = figurePosition;
        nextPosition = position;
        x = BASIC_X;
        y = BASIC_Y;
        nextX = x;
        nextY = y + 1;
        isUse = true;
    }

    void SetPosition(int figurePosition) { position = figurePosition; }

    void ShiftRight()
    {
        if (x < CURRENT_X2 - 7)
        {
            nextX = x + multiplier;
        }
        else
        {
            if (NEXT_POSITION_RIGHT[id].find(position) != NEXT_POSITION_RIGHT[id].end())
            {
                nextPosition = NEXT_POSITION_RIGHT[id].at(position);
            }
        }
    }

    void ShiftLeft()
    {
        if (x > CURRENT_X1)
        {
            nextX = x - multiplier;
        }
        else
        {
            if (NEXT_POSITION_LEFT[id].find(position) != NEXT_POSITION_LEFT[id].end())
            {
                nextPosition = NEXT_POSITION_LEFT[id].at(position);
            }
        }
    }

    void ShiftDown() { nextY = y + 1; }

    void RotateUp()
    {
        if (NEXT_POSITION_ROTATE_UP[id].find(position) != NEXT_POSITION_ROTATE_UP[id].end())
        {
            nextPosition = NEXT_POSITION_ROTATE_UP[id].at(position);
        }
    }

    void RotateDown()
    {
        if (NEXT_POSITION_ROTATE_DOWN[id].find(position) != NEXT_POSITION_ROTATE_DOWN[id].end())
        {
            nextPosition = NEXT_POSITION_ROTATE_DOWN[id].at(position);
        }
    }

    int GetPositionX(bool isNext = false) const { return isNext ? nextX : x; }

    int GetPositionY(bool isNext = false) const { return isNext ? nextY : y; }

    int GetId() const { return id; }

    int GetColorAsInt() const { return (int)color; }

    int GetPosition(bool isNext = false) const { return isNext ? nextPosition : position; }

    void Action(Actions action)
    {
        switch (action)
        {
        case Actions::LEFT:
            x = nextX;
            position = nextPosition;
            break;
        case Actions::RIGHT:
            x = nextX;
            position = nextPosition;
            break;
        case Actions::DOWN:
            y = nextY > 18 ? 18 : nextY;
            break;
        case Actions::ROTATE:
            position = nextPosition;
            break;
        case Actions::DEFAULT:
            break;
        }

        nextX = x;
        nextY = (y + 1) > 18 ? 18 : y + 1;
    }

    void SetUse(bool state) { isUse = state; }

    bool GetUse() const { return isUse; }
};

HANDLE hBasicBuffer;
HANDLE hDoubleBuffer;
SMALL_RECT bufferRect;
CHAR_INFO buffer[2000];
COORD bufferSize;
COORD bufferCoord;
BOOL fSuccess;
HWND hWnd;
RECT currentRect, basicRect;
CONSOLE_SCREEN_BUFFER_INFO csbi;
CONSOLE_FONT_INFOEX fontInfo;

void MyErrorExit(const wchar_t *comment);
void CreateGameField
(
    const Figure &nextFigure
    , const Figure &currentFigure
    , std::vector<std::pair<std::wstring, std::vector<int>>> &gameField
    , const std::vector<std::vector<int>> &gameStack
    , const unsigned int &score
    , const int &level
);
void ViewGameField(const std::vector<std::pair<std::wstring, std::vector<int>>> &gameField);
int PositionById(int figureId);
int StartMenu();
int ReturnToMenu();
void ProgramSettings();
void DoubleBufferingBegin();
void DoubleBufferingEnd();
bool PlayGame();
bool CheckCollisions(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> tempGameStack, KeyCodes key);
void SetFigure(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> &gameStack);
bool CheckGameOver(const std::vector<std::vector<std::pair<int, int>>> &gameStack);
void ClearKeyboardBuffer();
void ClearLines
(
    std::vector<std::vector<std::pair<int, int>>> &gameStack
    , const Figure &nextFigure
    , unsigned int &score
    , std::vector<std::pair<std::wstring, std::vector<int>>> &gameField
    , const int &level
);
void CheckScoreForLevel(const unsigned int &score, int &level);
void PlayMusic();

int main()
{
    ProgramSettings();

    do
    {
        system("cls");

        if (!StartMenu())
        {
            return 0;
        }
    }
    while (PlayGame());

    return 0;
}

void CreateGameField
(
    const Figure &nextFigure
    , const Figure &currentFigure
    , std::vector<std::pair<std::wstring, std::vector<int>>> &gameField
    , const std::vector<std::vector<std::pair<int, int>>> &gameStack
    , const unsigned int &score
    , const int &level
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
                SetConsoleTextAttribute(hBasicBuffer, (WORD)(i.second[j]));
                std::wcout << i.first[j];
                SetConsoleTextAttribute(hBasicBuffer, (WORD)(Colors::LIGHTGRAY));
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

int PositionById(int figureId)
{
    switch (figureId)
    {
    case 0:case 1:case 2:
        return rand() % 10;
    case 3:case 4:case 5:
        return rand() % 5;
    case 6:
        return rand() % 3;
    default:
        return -1;
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
        std::wcout << L"2 - Выход" << std::endl;
        menuItem = _getch() - '0';

        if (menuItem == 2)
        {
            return 0;
        }
        else
        {
            system("cls");

            if (menuItem == 1)
            {
                return menuItem;
            }
        }
    }
}

int ReturnToMenu()
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

    return menuItem;
}

void MyErrorExit(const wchar_t *comment)
{
    system("cls");
    std::wcout << comment;
    _getch();
    exit(0);
}

void ProgramSettings()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    srand(static_cast<unsigned>(time(NULL)));
    system("mode con cols=80 lines=25");
    hWnd = GetConsoleWindow();
    GetWindowRect(hWnd, &basicRect);
    hBasicBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
    hDoubleBuffer = CreateConsoleScreenBuffer
    (
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    if (hBasicBuffer == INVALID_HANDLE_VALUE ||
        hDoubleBuffer == INVALID_HANDLE_VALUE)
    {
        MyErrorExit(L"CreateConsoleScreenBuffer");
        return;
    }

    fontInfo.cbSize = sizeof(fontInfo);
    GetCurrentConsoleFontEx(hBasicBuffer, TRUE, &fontInfo);
    fontInfo.dwFontSize.Y = 24;
    SetCurrentConsoleFontEx(hBasicBuffer, TRUE, &fontInfo);
    GetCurrentConsoleFontEx(hDoubleBuffer, TRUE, &fontInfo);
    fontInfo.dwFontSize.Y = 24;
    SetCurrentConsoleFontEx(hDoubleBuffer, TRUE, &fontInfo);
    CONSOLE_CURSOR_INFO structCursorInfo;
    GetConsoleCursorInfo(hDoubleBuffer, &structCursorInfo);
    structCursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hDoubleBuffer, &structCursorInfo);

    GetConsoleCursorInfo(hBasicBuffer, &structCursorInfo);
    structCursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hBasicBuffer, &structCursorInfo);

    system("mode con cols=80 lines=25");
    hWnd = GetConsoleWindow();
    GetWindowRect(hWnd, &basicRect);

    if (!SetConsoleActiveScreenBuffer(hDoubleBuffer))
    {
        MyErrorExit(L"SetConsoleActiveScreenBuffer");
    }

    bufferSize.Y = 25;
    bufferSize.X = 80;

    bufferCoord.X = 0;
    bufferCoord.Y = 0;

    bufferRect.Top = 0;
    bufferRect.Left = 0;
    bufferRect.Bottom = 24;
    bufferRect.Right = 79;

    fSuccess = ReadConsoleOutput(
        hBasicBuffer,
        buffer,
        bufferSize,
        bufferCoord,
        &bufferRect);

    if (!fSuccess)
    {
        MyErrorExit(L"ReadConsoleOutput");
    }


    fSuccess = WriteConsoleOutput(
        hDoubleBuffer,
        buffer,
        bufferSize,
        bufferCoord,
        &bufferRect);

    if (!fSuccess)
    {
        MyErrorExit(L"WriteConsoleOutput");
    }

    if (!SetConsoleActiveScreenBuffer(hBasicBuffer))
    {
        MyErrorExit(L"SetConsoleActiveScreenBuffer");
    }
}

void DoubleBufferingBegin()
{
    GetWindowRect(hWnd, &currentRect);

    if (currentRect.right - currentRect.left != basicRect.right - basicRect.left
        || currentRect.bottom - currentRect.top != basicRect.bottom - basicRect.top)
    {
        SetConsoleScreenBufferSize(hBasicBuffer, csbi.dwSize);
        SetConsoleScreenBufferSize(hDoubleBuffer, csbi.dwSize);
        system("mode con cols=80 lines=25");
    }

    fSuccess = ReadConsoleOutput(
        hBasicBuffer,
        buffer,
        bufferSize,
        bufferCoord,
        &bufferRect);

    if (!fSuccess)
    {
        MyErrorExit(L"ReadConsoleOutput");
    }

    fSuccess = WriteConsoleOutput(
        hDoubleBuffer,
        buffer,
        bufferSize,
        bufferCoord,
        &bufferRect);

    if (!fSuccess)
    {
        MyErrorExit(L"WriteConsoleOutput");
    }

    SetConsoleActiveScreenBuffer(hDoubleBuffer);

    if (currentRect.right - currentRect.left != basicRect.right - basicRect.left
        || currentRect.bottom - currentRect.top != basicRect.bottom - basicRect.top)
    {
        system("mode con cols=80 lines=25");
    }
}

void DoubleBufferingEnd()
{
    SetConsoleActiveScreenBuffer(hBasicBuffer);
}

bool CheckCollisions(const Figure &currentFigure, std::vector<std::vector<std::pair<int, int>>> tempGameStack, KeyCodes key)
{
    bool isPositionNext = key == KeyCodes::UP_ARROW || key == KeyCodes::DOWN_ARROW
        || key == KeyCodes::LEFT_ARROW || key == KeyCodes::RIGHT_ARROW;
    bool isXNext = key == KeyCodes::LEFT_ARROW || key == KeyCodes::RIGHT_ARROW;
    bool isYNext = key == KeyCodes::SPACE && currentFigure.GetPositionY(true) != tempGameStack.size() - 3;
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
                ++tempGameStack[y][x].first;
                ++tempGameStack[y][x + 1].first;

                if (tempGameStack[y][x].first > 1 || tempGameStack[y][x + 1].first > 1)
                {
                    return true;
                }
            }
        }
    }

    return false;
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
    //PlayMusic();
    int levelGame = 1;
    unsigned int score = 0;
    int stepProgress;
    int figureId = rand() % FIGURES.size();
    int figurePosition = PositionById(figureId);
    Figure nextFigure(figureId, figurePosition);
    figureId = rand() % FIGURES.size();
    figurePosition = PositionById(figureId);
    Figure currentFigure;
    std::vector<std::vector<std::pair<int, int>>> gameStack(HEIGHT, std::vector<std::pair<int, int>>(WIDTH + 1, { 0,0 }));
    std::vector<std::pair<std::wstring, std::vector<int>>> gameField = GAME_FIELD;
    KeyBindings bindings = kDefaultBindings; // можно заменить на загрузку из настроек

    while (true)
    {
        ClearKeyboardBuffer();
        ClearLines(gameStack, nextFigure, score, gameField, levelGame);
        CheckScoreForLevel(score, levelGame);
        currentFigure = nextFigure;
        currentFigure.SetUse(true);
        figureId = rand() % FIGURES.size();
        figurePosition = PositionById(figureId);
        nextFigure.SetBasicParameters(figureId, figurePosition);
        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);

        if (CheckGameOver(gameStack))
        {
            return true;
        }

        for (int line = 0; line < 19; ++line)
        {
            stepProgress = 0;

            while (stepProgress < COUNT_STEP_CHAPTER)
            {
                if (IsKeyPressed(bindings.left))
                {
                    currentFigure.ShiftLeft();

                    if (!CheckCollisions(currentFigure, gameStack, KeyCodes::LEFT_ARROW))
                    {
                        currentFigure.Action(Actions::LEFT);
                    }
                    else
                    {
                        currentFigure.Action(Actions::DEFAULT);
                    }

                    CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                }
                else
                {
                    if (IsKeyPressed(bindings.right))
                    {
                        currentFigure.ShiftRight();

                        if (!CheckCollisions(currentFigure, gameStack, KeyCodes::RIGHT_ARROW))
                        {
                            currentFigure.Action(Actions::RIGHT);
                        }
                        else
                        {
                            currentFigure.Action(Actions::DEFAULT);
                        }

                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                    }
                    else
                    {
                        if (IsKeyPressed(bindings.rotate_up))
                        {
                            currentFigure.RotateUp();

                            if (!CheckCollisions(currentFigure, gameStack, KeyCodes::UP_ARROW))
                            {
                                currentFigure.Action(Actions::ROTATE);
                            }
                            else
                            {
                                currentFigure.Action(Actions::DEFAULT);
                            }

                            CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                        }
                        else
                        {
                            if (IsKeyPressed(bindings.rotate_down))
                            {
                                currentFigure.RotateDown();

                                if (!CheckCollisions(currentFigure, gameStack, KeyCodes::DOWN_ARROW))
                                {
                                    currentFigure.Action(Actions::ROTATE);
                                }
                                else
                                {
                                    currentFigure.Action(Actions::DEFAULT);
                                }

                                CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                            }
                            else
                            {
                                if (IsKeyPressed(bindings.hard_drop))
                                {
                                    if (line < 19)
                                    {
                                        currentFigure.ShiftDown();

                                        if (!CheckCollisions(currentFigure, gameStack, KeyCodes::SPACE))
                                        {
                                            currentFigure.Action(Actions::DOWN);

                                            if (line == 18)
                                            {
                                                SetFigure(currentFigure, gameStack);
                                                currentFigure.SetUse(false);
                                            }
                                        }
                                        else
                                        {
                                            SetFigure(currentFigure, gameStack);
                                            line = 19;
                                            break;
                                        }

                                        ++line;
                                        ++stepProgress;
                                        CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                                    }
                                }
                                else
                                {
                                    if (IsKeyPressed(bindings.exit))
                                    {
                                        switch (ReturnToMenu())
                                        {
                                        case 1: return true;
                                        case 2:
                                            {
                                                CreateGameField(nextFigure, currentFigure, gameField, gameStack, score, levelGame);
                                                break;
                                            }
                                        case 3: return false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Sleep(LEVEL_TIME_STEP[levelGame] / COUNT_STEP_CHAPTER);
                ++stepProgress;
            }

            if (line < 19)
            {
                currentFigure.ShiftDown();

                if (!CheckCollisions(currentFigure, gameStack, KeyCodes::SPACE))
                {
                    currentFigure.Action(Actions::DOWN);

                    if (line == 18)
                    {
                        SetFigure(currentFigure, gameStack);
                        currentFigure.SetUse(false);
                    }
                }
                else
                {
                    SetFigure(currentFigure, gameStack);
                    line = 19;
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
        _getch();
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
    std::vector<std::vector<std::pair<int, int>>> &gameStack
    , const Figure &nextFigure
    , unsigned int &score
    , std::vector<std::pair<std::wstring, std::vector<int>>> &gameField
    , const int &level
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

//void PlayMusic()
//{
//    sf::Music music;
//    music.openFromFile("ode.ogg");
//    music.play();
//
//    while (music.getStatus() == sf::Music::Playing)
//    {
//        sf::sleep(sf::milliseconds(100));
//    }
//}