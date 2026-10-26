#include "Console.h"
#include "../Core/GameData.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <shellapi.h>
#include <windows.h>
#include <wchar.h>

namespace Tetris
{
    ConsoleRenderer g_console;

    // Внутренняя функция: завершает программу с сообщением об ошибке.
    static void MyErrorExit(const wchar_t *comment)
    {
        system("cls");
        std::wcout << comment;
        (void)_getch(); // ждём нажатия, символ не нужен
        exit(0);
    }

    // Имя образа процесса по его PID ("" при ошибке).
    static std::wstring GetProcessImageName(DWORD pid)
    {
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (process == nullptr)
        {
            return L"";
        }

        wchar_t image[MAX_PATH]{};
        DWORD size = MAX_PATH;
        std::wstring name;

        if (QueryFullProcessImageNameW(process, 0, image, &size))
        {
            name = image;
        }

        (void)CloseHandle(process);
        return name;
    }

    // Запущены ли мы в conpty-хосте (Windows Terminal, терминал VS Code
    // и т.п.): окно консоли имеет класс PseudoConsoleWindow, у классической
    // консоли (conhost) — ConsoleWindowClass. Не зависит от переменных
    // окружения — работает и при запуске с повышенными правами.
    static bool IsConptyHost()
    {
        wchar_t windowClass[64]{};
        if (GetClassNameW(g_console.hWnd, windowClass, 64) == 0)
        {
            return false;
        }

        return wcscmp(windowClass, L"PseudoConsoleWindow") == 0;
    }

    // Определяет, запущены ли мы в conpty-хосте (нужен адаптивный режим).
    // Сначала — переменные окружения (WT_SESSION задаёт WT всем хостируемым
    // процессам, TERM_PROGRAM — запасной вариант); затем — класс окна
    // консоли (защита от случаев, когда окружение не пробрасывается,
    // например при запуске с повышенными правами).
    static bool DetectConptyHost()
    {
        wchar_t value[64]{};

        // WT_SESSION задаётся WT и всегда непустая — достаточно наличия.
        if (GetEnvironmentVariableW(L"WT_SESSION", value, 64) > 0)
        {
            return true;
        }

        if (GetEnvironmentVariableW(L"TERM_PROGRAM", value, 64) > 0
            && wcscmp(value, L"WindowsTerminal") == 0)
        {
            return true;
        }

        return IsConptyHost();
    }

    // Смещение кадра 80x25 внутри фактического буфера (адаптивный режим):
    // кадр центрируется, если буфер меньше кадра — смещение 0 и кадр
    // обрезается справа/снизу. В фиксированном режиме — всегда (0,0).
    static void ComputeFrameOffset()
    {
        if (g_console.mode == ConsoleMode::Fixed)
        {
            g_console.frameOffset.X = 0;
            g_console.frameOffset.Y = 0;
            return;
        }

        g_console.frameOffset.X = (g_console.csbi.dwSize.X > 80)
            ? static_cast<SHORT>((g_console.csbi.dwSize.X - 80) / 2)
            : 0;
        g_console.frameOffset.Y = (g_console.csbi.dwSize.Y > 25)
            ? static_cast<SHORT>((g_console.csbi.dwSize.Y - 25) / 2)
            : 0;
    }

    // Вычисляет область копирования между буферами: пересечение кадра (80x25,
    // смещённого на frameOffset) с фактическим размером буфера. В фиксированном
    // режиме — всегда 80x25 от (0,0); в адаптивном — если буфер меньше кадра,
    // область сужается до буфера.
    static void ComputeCopyRegion()
    {
        ComputeFrameOffset();

        int copyW = 80;
        int copyH = 25;

        if (g_console.mode == ConsoleMode::Adaptive)
        {
            if (g_console.csbi.dwSize.X < 80)
            {
                copyW = static_cast<int>(g_console.csbi.dwSize.X);
            }

            if (g_console.csbi.dwSize.Y < 25)
            {
                copyH = static_cast<int>(g_console.csbi.dwSize.Y);
            }
        }

        g_console.bufferSize.X = copyW;
        g_console.bufferSize.Y = copyH;
        g_console.bufferCoord.X = g_console.frameOffset.X;
        g_console.bufferCoord.Y = g_console.frameOffset.Y;
        g_console.bufferRect.Left = 0;
        g_console.bufferRect.Top = 0;
        g_console.bufferRect.Right = copyW - 1;
        g_console.bufferRect.Bottom = copyH - 1;
    }

    void ProgramSettings()
    {
        (void)_setmode(_fileno(stdout), _O_U16TEXT);
        srand(static_cast<unsigned>(time(NULL)));
        g_console.hWnd = GetConsoleWindow();
        GetWindowRect(g_console.hWnd, &g_console.basicRect);
        g_console.basicBuffer = GetStdHandle(STD_OUTPUT_HANDLE);

        g_console.doubleBuffer = CreateConsoleScreenBuffer(
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

        // Режим рендера: по умолчанию — conpty-хост (Windows Terminal и
        // т.п.) адаптивный, иначе фиксированный.
        g_console.mode = DetectConptyHost()
            ? ConsoleMode::Adaptive
            : ConsoleMode::Fixed;

        // Truecolor (24-бит, VT) — только Adaptive (Windows Terminal):
        // рисуем напрямую в видимый буфер, truecolor там удерживается.
        // В Fixed конвейер идёт через CHAR_INFO (16 атрибутов), 24-бит
        // не удержится — остаёмся на 16-цветном пути (SetConsoleTextAttribute).
        if (g_console.mode == ConsoleMode::Adaptive)
        {
            DWORD consoleMode = 0;
            if (GetConsoleMode(g_console.basicBuffer, &consoleMode))
            {
                if (SetConsoleMode(g_console.basicBuffer, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
                {
                    g_console.truecolor = true;
                }
            }
        }

        // ВРЕМЕННО (отладка): флагами командной строки можно переопределить
        // режим (--adaptive / --fixed) и записать диагностику в
        // tetris_diag.log (--diag). Убрать после проверки Adaptive-пути.
        bool writeDiag = false;
        int argCount = 0;
        LPWSTR *argList = CommandLineToArgvW(GetCommandLineW(), &argCount);
        if (argList != nullptr)
        {
            for (int i = 1; i < argCount; ++i)
            {
                if (wcscmp(argList[i], L"--adaptive") == 0)
                {
                    g_console.mode = ConsoleMode::Adaptive;
                }
                else if (wcscmp(argList[i], L"--fixed") == 0)
                {
                    g_console.mode = ConsoleMode::Fixed;
                }
                else if (wcscmp(argList[i], L"--diag") == 0)
                {
                    writeDiag = true;
                }
            }

            (void)LocalFree(argList);
        }

        // Шрифт 24px — только в фиксированном режиме (conhost), где мы сами
        // управляем консолью. В адаптивном режиме (conpty/Windows Terminal)
        // шрифт контролирует терминал: принудительная смена не работает и
        // сбрасывает буфер к дефолтным 80x25, из-за чего кадр не центрируется
        // и не видно реального размера окна.
        if (g_console.mode == ConsoleMode::Fixed)
        {
            CONSOLE_FONT_INFOEX fontInfo{};
            fontInfo.cbSize = sizeof(fontInfo);

            GetCurrentConsoleFontEx(g_console.basicBuffer, TRUE, &fontInfo);
            fontInfo.dwFontSize.Y = 24;
            SetCurrentConsoleFontEx(g_console.basicBuffer, TRUE, &fontInfo);

            GetCurrentConsoleFontEx(g_console.doubleBuffer, TRUE, &fontInfo);
            fontInfo.dwFontSize.Y = 24;
            SetCurrentConsoleFontEx(g_console.doubleBuffer, TRUE, &fontInfo);
        }

        CONSOLE_CURSOR_INFO structCursorInfo;

        GetConsoleCursorInfo(g_console.doubleBuffer, &structCursorInfo);
        structCursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(g_console.doubleBuffer, &structCursorInfo);

        GetConsoleCursorInfo(g_console.basicBuffer, &structCursorInfo);
        structCursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(g_console.basicBuffer, &structCursorInfo);

        // Размер: в фиксированном режиме — как раньше (mode con).
        // В адаптивном режиме размер не форсируем — в Windows Terminal
        // буфер следует за окном, фактический размер прочитаем ниже.
        if (g_console.mode == ConsoleMode::Fixed)
        {
            system("mode con cols=80 lines=25");
        }

        g_console.hWnd = GetConsoleWindow();
        GetWindowRect(g_console.hWnd, &g_console.basicRect);

        // Фактический размер основного буфера: на нём строится область
        // копирования и (в адаптивном режиме) подсказка о размере окна.
        if (!GetConsoleScreenBufferInfo(g_console.basicBuffer, &g_console.csbi))
        {
            MyErrorExit(L"GetConsoleScreenBufferInfo");
        }

        // Область копирования (размер и смещение кадра) — в ComputeCopyRegion.
        ComputeCopyRegion();
        // ВРЕМЕННО (отладка): диагностика в tetris_diag.log (--diag).
        // Пишем после вычисления csbi и frameOffset, чтобы логировать
        // фактические размеры буфера, видимого окна и смещения кадра.
        if (writeDiag)
        {
            wchar_t wtSession[64]{};
            wchar_t termProgram[64]{};
            (void)GetEnvironmentVariableW(L"WT_SESSION", wtSession, 64);
            (void)GetEnvironmentVariableW(L"TERM_PROGRAM", termProgram, 64);

            wchar_t exePath[MAX_PATH]{};
            (void)GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            wchar_t *lastSlash = wcsrchr(exePath, L'\\');
            if (lastSlash != nullptr)
            {
                *lastSlash = L'\0';
            }

            std::wstring logPath = std::wstring(exePath) + L"\\tetris_diag.log";
            std::wofstream diagLog(logPath.c_str());
            if (diagLog)
            {
                wchar_t windowClass[64]{};
                (void)GetClassNameW(g_console.hWnd, windowClass, 64);
                DWORD windowPid = 0;
                (void)GetWindowThreadProcessId(g_console.hWnd, &windowPid);
                std::wstring windowOwner = GetProcessImageName(windowPid);

                int winW = g_console.csbi.srWindow.Right - g_console.csbi.srWindow.Left + 1;
                int winH = g_console.csbi.srWindow.Bottom - g_console.csbi.srWindow.Top + 1;

                diagLog << L"WT_SESSION=\"" << wtSession << L"\"\n"
                    << L"TERM_PROGRAM=\"" << termProgram << L"\"\n"
                    << L"window_class=\"" << windowClass << L"\"\n"
                    << L"window_owner=\"" << windowOwner << L"\"\n"
                    << L"buffer_size=" << g_console.csbi.dwSize.X << L"x" << g_console.csbi.dwSize.Y << L"\n"
                    << L"visible_window=" << winW << L"x" << winH << L"\n"
                    << L"frame_offset=" << g_console.frameOffset.X << L"," << g_console.frameOffset.Y << L"\n"
                    << L"mode=" << (g_console.mode == ConsoleMode::Adaptive ? L"Adaptive" : L"Fixed") << L"\n";
            }
        }

        // Фиксированный режим: первичная синхронизация резервного буфера
        // с основным. В адаптивном режиме двойная буферизация не используется
        // (рисуем напрямую в видимый буфер), синхронизировать нечего.
        if (g_console.mode == ConsoleMode::Fixed)
        {
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
        }
    }

    void DoubleBufferingBegin()
    {
        if (g_console.mode == ConsoleMode::Adaptive)
        {
            // Адаптивный режим: рисуем напрямую в видимый буфер, без двойной
            // буферизации. Копии и переключение буферов в conpty дают
            // артефакты и мерцание: кадр пишется в один буфер, а показывается
            // другой. Видимый буфер (basicBuffer) остаётся активным.
            UpdateAdaptiveBuffer();
            return;
        }

        // Фиксированный режим: как раньше.
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
        if (g_console.mode == ConsoleMode::Adaptive)
        {
            return; // рисуем напрямую в видимый буфер, переключать нечего
        }

        SetConsoleActiveScreenBuffer(g_console.basicBuffer);
    }

    void UpdateAdaptiveBuffer()
    {
        // Адаптивный режим: основной буфер следует за окном (conpty),
        // поэтому перечитываем фактический размер и пересчитываем
        // смещение кадра (центрирование). При ресайзе очищаем буфер —
        // иначе старый кадр, нарисованный со старым смещением, остаётся
        // артефактом вне области нового кадра.
        if (!GetConsoleScreenBufferInfo(g_console.basicBuffer, &g_console.csbi))
        {
            MyErrorExit(L"GetConsoleScreenBufferInfo");
        }

        ComputeCopyRegion();

        if (g_console.lastBufSize.X != g_console.csbi.dwSize.X
            || g_console.lastBufSize.Y != g_console.csbi.dwSize.Y)
        {
            g_console.lastBufSize = g_console.csbi.dwSize;

            ClearConsoleBuffer();
        }
    }

    void ClearConsoleBuffer()
    {
        if (g_console.mode != ConsoleMode::Adaptive)
        {
            return; // фиксированный режим: кадр = весь буфер, хвостов нет
        }

        // Очистка всего буфера пробелами: линейно от (0,0) с переносом
        // по ширине консоли, всего width*height ячеек. Последний
        // параметр — указатель на счётчик записанных символов (LPDWORD).
        const DWORD totalCells = static_cast<DWORD>(
            g_console.csbi.dwSize.X * g_console.csbi.dwSize.Y);
        DWORD written;
        (void)FillConsoleOutputCharacterW(
            g_console.basicBuffer, L' ', totalCells, COORD{ 0, 0 }, &written);
    }

    // Truecolor (24-бит, только Adaptive): добавляет в строку
    // VT-последовательность цвета ESC[38;2;R;G;Bm. rgb — индекс в kColorRgb.
    // Строка собирается целиком и выводится одним записом (быстрее, чем
    // по-символьно через std::wcout).
    void AppendTrueColor(std::wstring &row, int rgb)
    {
        const Rgb &c = kColorRgb[rgb];
        row += L"\x1b[38;2;";
        row += std::to_wstring(c.r);
        row += L";";
        row += std::to_wstring(c.g);
        row += L";";
        row += std::to_wstring(c.b);
        row += L"m";
    }

    // Truecolor: добавляет в строку VT-последовательность установки
    // позиции курсора (CUP, ESC[row;colH). row, col — 1-based.
    void AppendCursorPos(std::wstring &out, int row, int col)
    {
        out += L"\x1b[";
        out += std::to_wstring(row);
        out += L";";
        out += std::to_wstring(col);
        out += L"H";
    }
}
