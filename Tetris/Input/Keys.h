#pragma once

#include <cstddef>
#include <cstdint>

#include <windows.h>

namespace Tetris
{
    // Виртуальные коды клавиш (VK_*), используемые для опроса состояния ввода.
    enum class KeyCodes
        : uint8_t
    {
        // Контрольные клавиши
        BACKSPACE = 8,
        TAB = 9,
        ENTER = 13,
        SHIFT = 16,
        CTRL = 17,
        ALT = 18,
        PAUSE_BREAK = 19,
        CAPS_LOCK = 20,
        ESCAPE = 0x1B,

        // Навигация
        PAGE_UP = 33,
        PAGE_DOWN = 34,
        END = 35,
        HOME = 36,
        LEFT_ARROW = 37,
        UP_ARROW = 38,
        RIGHT_ARROW = 39,
        DOWN_ARROW = 40,
        INSERT = 45,
        DEL = 46,

        // Печать (буквы, цифры, знаки)
        DIGIT_0 = '0',
        DIGIT_1 = '1',
        DIGIT_2 = '2',
        DIGIT_3 = '3',
        DIGIT_4 = '4',
        DIGIT_5 = '5',
        DIGIT_6 = '6',
        DIGIT_7 = '7',
        DIGIT_8 = '8',
        DIGIT_9 = '9',
        A = 'A',
        B = 'B',
        C = 'C',
        D = 'D',
        E = 'E',
        F = 'F',
        G = 'G',
        H = 'H',
        I = 'I',
        J = 'J',
        K = 'K',
        L = 'L',
        M = 'M',
        N = 'N',
        O = 'O',
        P = 'P',
        Q = 'Q',
        R = 'R',
        S = 'S',
        T = 'T',
        U = 'U',
        V = 'V',
        W = 'W',
        X = 'X',
        Y = 'Y',
        Z = 'Z',
        SPACE = ' ',
        SEMI_COLON = 186,
        EQUAL_SIGN = 187,
        COMMA = 188,
        DASH = 189,
        PERIOD = 190,
        FORWARD_SLASH = 191,
        GRAVE_ACCENT = 192,
        OPEN_BRACKET = 0xDB,
        BACK_SLASH = 0xDC,
        CLOSE_BRAKET = 0xDD,
        SINGLE_QUOTE = 222,

        // Системные
        LEFT_WINDOW = 91,
        RIGHT_WINDOW = 92,
        SELECT_KEY = 93,

        // Числовая клавиатура
        NUMPAD_0 = 96,
        NUMPAD_1 = 97,
        NUMPAD_2 = 98,
        NUMPAD_3 = 99,
        NUMPAD_4 = 100,
        NUMPAD_5 = 101,
        NUMPAD_6 = 102,
        NUMPAD_7 = 103,
        NUMPAD_8 = 104,
        NUMPAD_9 = 105,
        MULTIPLY = 106,
        ADD = 107,
        SUBTRACT = 109,
        DECIMAL_POINT = 110,
        DIVIDE = 111,

        // Функциональные клавиши
        F1 = 112,
        F2 = 113,
        F3 = 114,
        F4 = 115,
        F5 = 116,
        F6 = 117,
        F7 = 118,
        F8 = 119,
        F9 = 120,
        F10 = 121,
        F11 = 122,
        F12 = 123,
        NUM_LOCK = 144,
        SCROLL_LOCK = 145
    };

    // Действия игры, которым назначаются клавиши.
    // Порядок совпадает с порядком элементов KeyBindings::keys.
    enum class BindingAction
        : unsigned char
    {
        LEFT = 0,        // Движение влево
        RIGHT = 1,       // Движение вправо
        ROTATE_UP = 2,   // Поворот (клавиша вверх)
        ROTATE_DOWN = 3, // Поворот (клавиша вниз)
        HARD_DROP = 4,   // Мгновенный сброс фигуры
        EXIT = 5,        // Возврат в меню
        COUNT            // Не действие, а количество действий
    };

    // Набор клавиш, управляющих игрой.
    // Массив (а не именованные поля) упрощает сериализацию
    // в файл настроек и обход по действиям.
    struct KeyBindings
    {
        KeyCodes keys[static_cast<std::size_t>(BindingAction::COUNT)];
    };

    // Возвращает true, если клавиша сейчас нажата.
    // Проверяется задокументированный старший бит «нажатия» (0x8000);
    // младшие 15 бит содержат счётчик автоповтора и проверять их нельзя.
    inline bool IsKeyPressed(KeyCodes key) noexcept
    {
        return (GetAsyncKeyState(static_cast<UINT>(key)) & 0x8000) != 0;
    }
}
