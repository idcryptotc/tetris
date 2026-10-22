#pragma once

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

    extern ConsoleRenderer g_console;

    // Инициализация консоли (размер, шрифт, unicode-режим, двойная буферизация).
    void ProgramSettings();

    // Начало кадра: копирует текущий экран в резервный буфер и переключает на него.
    void DoubleBufferingBegin();

    // Конец кадра: возвращает активный буфер на основной (видимый).
    void DoubleBufferingEnd();
}
