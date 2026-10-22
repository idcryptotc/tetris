#include "Console.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>

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

    void ProgramSettings()
    {
        (void)_setmode(_fileno(stdout), _O_U16TEXT);
        srand(static_cast<unsigned>(time(NULL)));
        system("mode con cols=80 lines=25");
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
}
