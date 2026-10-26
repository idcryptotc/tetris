#pragma once

#include <Windows.h>

#include <string>

namespace Tetris
{
    // Режим рендера консоли.
    enum class ConsoleMode
    {
        Fixed,    // Фиксированные 80x25 через mode con (Win10/conhost)
        Adaptive  // Адаптация под размер окна (Windows Terminal)
    };

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
        ConsoleMode mode{ ConsoleMode::Fixed };         // Режим рендера
        COORD frameOffset{ 0, 0 };                      // Смещение кадра в буфере (Adaptive)
        COORD lastBufSize{ 0, 0 };                      // Размер буфера прошлого кадра (Adaptive: очистка при ресайзе)
        bool truecolor{ false };                        // Truecolor (VT) включён (только Adaptive)
        std::wstring prevFrame;                         // Предыдущий truecolor-кадр (VT-строка) для показа в резервном буфере
    };

    extern ConsoleRenderer g_console;

    // Инициализация консоли (размер, шрифт, unicode-режим, двойная буферизация).
    void ProgramSettings();

    // Адаптивный режим: читает фактический размер буфера (следует за окном в WT),
    // при ресайзе ресайзит оба буфера, очищает их, сбрасывает prevFrame
    // (старые CUP-смещения неактуальны) и вычисляет смещение кадра
    // (центрирование). Вызывается перед отрисовкой кадра.
    void UpdateAdaptiveBuffer();

    // Адаптивный режим: очищает оба буфера. Нужно при переходе из меню
    // паузы в игру: кадр центрируется и не покрывает весь буфер, поэтому
    // текст меню в левом верхнем углу остаётся видимым. В фиксированном
    // режиме — no-op (кадр = весь буфер, хвостов нет).
    void ClearConsoleBuffer();

    // Начало кадра: показывает предыдущий кадр в резервном буфере и переключает
    // на него; новый кадр рисуется в основной (видимый) буфер. В адаптивном
    // режиме предыдущий кадр показывается через WriteConsoleW (сохранённая
    // VT-строка prevFrame); в фиксированном — CHAR_INFO-копия.
    void DoubleBufferingBegin();

    // Конец кадра: возвращает активный буфер на основной (видимый).
    void DoubleBufferingEnd();

    // Truecolor (24-бит, только Adaptive): добавляет в строку
    // VT-последовательность цвета ESC[38;2;R;G;Bm. rgb — индекс в kColorRgb.
    void AppendTrueColor(std::wstring &row, int rgb);

    // 16-цвет (только Adaptive): добавляет в строку VT-последовательность
    // цвета ESC[38;5;Nm (ANSI 16, палитра терминала). color — индекс
    // в Colors (0..15), как в SetConsoleTextAttribute.
    void AppendAnsi16Color(std::wstring &row, int color);

    // Truecolor: добавляет в строку VT-последовательность установки
    // позиции курсора (CUP, ESC[row;colH). row, col — 1-based.
    void AppendCursorPos(std::wstring &out, int row, int col);
}
