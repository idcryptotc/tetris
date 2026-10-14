#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>

#include <conio.h>
#include <windows.h>

#include "Keys.h"

namespace Tetris
{
    // Имя файла настроек (создаётся в директории запуска программы).
    inline constexpr const wchar_t* kSettingsFileName = L"tetris_settings.ini";

    // Набор клавиш по умолчанию (используется, если файла настроек нет).
    inline constexpr KeyBindings kDefaultBindings
    {
        {
            KeyCodes::LEFT_ARROW,  // LEFT
            KeyCodes::RIGHT_ARROW, // RIGHT
            KeyCodes::UP_ARROW,    // ROTATE_UP
            KeyCodes::DOWN_ARROW,  // ROTATE_DOWN
            KeyCodes::SPACE,       // HARD_DROP
            KeyCodes::ESCAPE       // EXIT
        }
    };

    // Текущий набор клавиш: загружается в начале программы,
    // меняется только в меню настроек (между партиями).
    inline KeyBindings g_bindings = kDefaultBindings;

    // Клавиша, назначенная на действие (удобный доступ к g_bindings).
    inline KeyCodes KeyFor(BindingAction action) noexcept
    {
        return g_bindings.keys[static_cast<std::size_t>(action)];
    }

    // Пара «действие -> ключ файла настроек + подпись в меню».
    // Порядок совпадает с BindingAction (см. Keys.h).
    struct BindingKey
    {
        BindingAction action;
        const wchar_t* keyName; // ключ в INI-файле
        const wchar_t* label;   // подпись в меню настройки
    };

    inline constexpr BindingKey kBindingKeys[]
    {
        { BindingAction::LEFT,        L"Left",       L"Влево" },
        { BindingAction::RIGHT,       L"Right",      L"Вправо" },
        { BindingAction::ROTATE_UP,   L"RotateUp",   L"Поворот вверх" },
        { BindingAction::ROTATE_DOWN, L"RotateDown", L"Поворот вниз" },
        { BindingAction::HARD_DROP,   L"HardDrop",   L"Сброс вниз" },
        { BindingAction::EXIT,        L"Exit",       L"Выход в меню" },
    };

    // Человекочитаемое имя клавиши по её коду (для меню и INI-файла).
    // Буквы и цифры записываются как KEY_A, KEY_0 и т.д.
    inline std::wstring KeyName(KeyCodes key) noexcept
    {
        switch (static_cast<unsigned char>(key))
        {
            case static_cast<unsigned char>(KeyCodes::BACKSPACE):   return L"BACKSPACE";
            case static_cast<unsigned char>(KeyCodes::TAB):         return L"TAB";
            case static_cast<unsigned char>(KeyCodes::ENTER):       return L"ENTER";
            case static_cast<unsigned char>(KeyCodes::SHIFT):       return L"SHIFT";
            case static_cast<unsigned char>(KeyCodes::CTRL):        return L"CTRL";
            case static_cast<unsigned char>(KeyCodes::ALT):         return L"ALT";
            case static_cast<unsigned char>(KeyCodes::ESCAPE):      return L"ESCAPE";
            case static_cast<unsigned char>(KeyCodes::PAGE_UP):     return L"PAGEUP";
            case static_cast<unsigned char>(KeyCodes::PAGE_DOWN):   return L"PGDN";
            case static_cast<unsigned char>(KeyCodes::END):         return L"END";
            case static_cast<unsigned char>(KeyCodes::HOME):        return L"HOME";
            case static_cast<unsigned char>(KeyCodes::LEFT_ARROW):  return L"LEFT";
            case static_cast<unsigned char>(KeyCodes::UP_ARROW):    return L"UP";
            case static_cast<unsigned char>(KeyCodes::RIGHT_ARROW): return L"RIGHT";
            case static_cast<unsigned char>(KeyCodes::DOWN_ARROW):  return L"DOWN";
            case static_cast<unsigned char>(KeyCodes::INSERT):      return L"INSERT";
            case static_cast<unsigned char>(KeyCodes::DEL):         return L"DEL";
            case static_cast<unsigned char>(KeyCodes::SPACE):       return L"SPACE";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_0):    return L"NUMPAD0";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_1):    return L"NUMPAD1";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_2):    return L"NUMPAD2";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_3):    return L"NUMPAD3";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_4):    return L"NUMPAD4";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_5):    return L"NUMPAD5";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_6):    return L"NUMPAD6";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_7):    return L"NUMPAD7";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_8):    return L"NUMPAD8";
            case static_cast<unsigned char>(KeyCodes::NUMPAD_9):    return L"NUMPAD9";
            case static_cast<unsigned char>(KeyCodes::F1):          return L"F1";
            case static_cast<unsigned char>(KeyCodes::F2):          return L"F2";
            case static_cast<unsigned char>(KeyCodes::F3):          return L"F3";
            case static_cast<unsigned char>(KeyCodes::F4):          return L"F4";
            case static_cast<unsigned char>(KeyCodes::F5):          return L"F5";
            case static_cast<unsigned char>(KeyCodes::F6):          return L"F6";
            case static_cast<unsigned char>(KeyCodes::F7):          return L"F7";
            case static_cast<unsigned char>(KeyCodes::F8):          return L"F8";
            case static_cast<unsigned char>(KeyCodes::F9):          return L"F9";
            case static_cast<unsigned char>(KeyCodes::F10):         return L"F10";
            case static_cast<unsigned char>(KeyCodes::F11):         return L"F11";
            case static_cast<unsigned char>(KeyCodes::F12):         return L"F12";
            default: break;
        }

        if (key >= KeyCodes::DIGIT_0 && key <= KeyCodes::DIGIT_9)
        {
            return std::wstring(L"KEY_") +
                   static_cast<wchar_t>(L'0' + static_cast<unsigned char>(key) - 48u);
        }
        if (key >= KeyCodes::A && key <= KeyCodes::Z)
        {
            return std::wstring(L"KEY_") +
                   static_cast<wchar_t>(L'A' + static_cast<unsigned char>(key) - 65u);
        }
        return L"UNKNOWN";
    }

    // Таблица «имя клавиши -> VK-код» для разбора INI-файла.
    // Строится один раз при первом обращении.
    inline const std::unordered_map<std::wstring, int>& KeyNameTable() noexcept
    {
        static const std::unordered_map<std::wstring, int> table = []
        {
            std::unordered_map<std::wstring, int> t;
            auto add = [&t](const wchar_t* name, int vk) { t.emplace(name, vk); };

            add(L"BACKSPACE", 8);
            add(L"TAB", 9);
            add(L"ENTER", 13);
            add(L"SHIFT", 16);
            add(L"CTRL", 17);
            add(L"ALT", 18);
            add(L"ESCAPE", 27);
            add(L"PAGEUP", 33);
            add(L"PGDN", 34);
            add(L"END", 35);
            add(L"HOME", 36);
            add(L"LEFT", 37);
            add(L"UP", 38);
            add(L"RIGHT", 39);
            add(L"DOWN", 40);
            add(L"INSERT", 45);
            add(L"DEL", 46);
            add(L"SPACE", 32);
            add(L"NUMPAD0", 96); add(L"NUMPAD1", 97); add(L"NUMPAD2", 98);
            add(L"NUMPAD3", 99); add(L"NUMPAD4", 100); add(L"NUMPAD5", 101);
            add(L"NUMPAD6", 102); add(L"NUMPAD7", 103); add(L"NUMPAD8", 104);
            add(L"NUMPAD9", 105);
            add(L"F1", 112); add(L"F2", 113); add(L"F3", 114); add(L"F4", 115);
            add(L"F5", 116); add(L"F6", 117); add(L"F7", 118); add(L"F8", 119);
            add(L"F9", 120); add(L"F10", 121); add(L"F11", 122); add(L"F12", 123);

            for (int i = 0; i < 10; ++i)
            {
                t.emplace(std::wstring(L"KEY_") + static_cast<wchar_t>('0' + i), '0' + i);
            }
            for (int i = 0; i < 26; ++i)
            {
                t.emplace(std::wstring(L"KEY_") + static_cast<wchar_t>('A' + i), 'A' + i);
            }
            return t;
        }();
        return table;
    }

    // Код клавиши по имени из файла настроек; -1, если имя не распознано.
    inline int KeyFromName(const std::wstring& name) noexcept
    {
        const auto& table = KeyNameTable();
        auto it = table.find(name);
        return it == table.end() ? -1 : it->second;
    }

    // Загружает настройки из INI-файла; отсутствующие или некорректные
    // значения заменяются значениями по умолчанию.
    inline void LoadBindings(KeyBindings& bindings)
    {
        for (std::size_t i = 0; i < static_cast<std::size_t>(BindingAction::COUNT); ++i)
        {
            // Выход в меню всегда на ESC (см. ConfigureBindings).
            if (kBindingKeys[i].action == BindingAction::EXIT)
            {
                continue;
            }

            wchar_t buffer[64] = {};
            if (GetPrivateProfileStringW(
                    L"Bindings",
                    kBindingKeys[i].keyName,
                    L"",
                    buffer,
                    static_cast<UINT>(sizeof(buffer) / sizeof(buffer[0])),
                    kSettingsFileName) > 0)
            {
                int code = KeyFromName(buffer);
                if (code > 0)
                {
                    bindings.keys[i] = static_cast<KeyCodes>(code);
                }
            }
        }
    }

    // Сохраняет настройки в INI-файл (создаёт/перезаписывает).
    inline bool SaveBindings(const KeyBindings& bindings)
    {
        for (std::size_t i = 0; i < static_cast<std::size_t>(BindingAction::COUNT); ++i)
        {
            if (WritePrivateProfileStringW(
                    L"Bindings",
                    kBindingKeys[i].keyName,
                    KeyName(bindings.keys[i]).c_str(),
                    kSettingsFileName) == 0)
            {
                return false;
            }
        }
        return true;
    }

    // Очищает клавиатурный буфер.
    inline void FlushKeyboard() noexcept
    {
        while (_kbhit())
        {
            _getch();
        }
    }

    // Ждёт нажатия клавиши и возвращает её VK-код.
    // ESC — отмена (возвращает 0).
    //
    // Консольное приложение: нажатия не попадают в очередь сообщений
    // процесса (PeekMessageW тут бесполезен и виснет), а ReadConsoleInputW
    // в свежем SDK имеет нестандартную структуру INPUT_RECORD.
    // Поэтому опрашиваем глобальное состояние клавиатуры через
    // GetAsyncKeyState — тот же механизм, что и в игровом цикле.
    inline unsigned CaptureKey()
    {
        FlushKeyboard();
        std::wcout << L"Нажмите клавишу... (ESC - отменить)" << std::endl;
        std::wcout.flush();

        // Модификаторы и системные состояния нельзя назначать на действия.
        auto isBindable = [](UINT vk) noexcept
        {
            switch (vk)
            {
                case VK_SHIFT:
                case VK_CONTROL:
                case VK_MENU:
                case VK_CAPITAL:
                case VK_NUMLOCK:
                case VK_SCROLL:
                case VK_PAUSE:
                case VK_LWIN:
                case VK_RWIN:
                case VK_SELECT:
                    return false;
                default:
                    return true;
            }
        };

        // Снимок клавиш, уже удерживаемых в момент запроса:
        // они не должны считаться ответом.
        bool wasPressed[256] = {};
        for (UINT vk = 0; vk < 256; ++vk)
        {
            if (isBindable(vk))
            {
                wasPressed[vk] = (GetAsyncKeyState(static_cast<INT>(vk)) & 0x8000) != 0;
            }
        }

        // Ждём, пока новая (не удерживавшаяся) клавиша окажется нажатой.
        while (true)
        {
            // ESC — отмена.
            if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
            {
                FlushKeyboard();
                return 0;
            }

            for (UINT vk = 1; vk < 256; ++vk)
            {
                if (!isBindable(vk) || wasPressed[vk])
                {
                    continue;
                }
                if ((GetAsyncKeyState(static_cast<INT>(vk)) & 0x8000) != 0)
                {
                    // Захваченное нажатие остаётся в буфере ввода консоли;
                    // сливаем его, чтобы оно не попало в следующий _getch().
                    FlushKeyboard();
                    return vk;
                }
            }

            Sleep(20);
        }
    }

    // Интерактивная настройка клавиш: по очереди запрашивает
    // новую клавишу для каждого действия.
    // Возвращает true, если изменения были сохранены.
    inline bool ConfigureBindings(KeyBindings& bindings)
    {
        std::wcout << L"Настройка клавиш" << std::endl;
        std::wcout << L"==================" << std::endl;

        bool changed = false;

        for (std::size_t i = 0; i < static_cast<std::size_t>(BindingAction::COUNT); ++i)
        {
            std::wcout << L"\n" << kBindingKeys[i].label << L": "
                       << KeyName(bindings.keys[i]) << std::endl;

            // Выход в меню всегда на ESC: он зарезервирован под отмену
            // в самом меню настроек, поэтому переназначению не подлежит.
            if (kBindingKeys[i].action == BindingAction::EXIT)
            {
                std::wcout << L"  (закреплена за ESC, не переназначается)" << std::endl;
                continue;
            }

            unsigned vk = CaptureKey();
            if (vk == 0)
            {
                std::wcout << L"  (без изменений)" << std::endl;
                continue;
            }

            KeyCodes newKey = static_cast<KeyCodes>(vk);
            if (newKey == bindings.keys[i])
            {
                std::wcout << L"  (без изменений)" << std::endl;
                continue;
            }

            // Одна клавиша не может быть назначена на два действия.
            bool conflict = false;
            for (std::size_t j = 0; j < static_cast<std::size_t>(BindingAction::COUNT); ++j)
            {
                if (j != i && bindings.keys[j] == newKey)
                {
                    std::wcout << L"  ! Клавиша уже назначена: "
                               << kBindingKeys[j].label << std::endl;
                    conflict = true;
                    break;
                }
            }

            if (!conflict)
            {
                bindings.keys[i] = newKey;
                changed = true;
                std::wcout << L"  -> " << KeyName(newKey) << std::endl;
            }
        }

        if (!changed)
        {
            std::wcout << L"\nИзменений нет." << std::endl;
            return false;
        }

        // Подтверждение через Enter/Esc — не зависит от раскладки клавиатуры
        // (в отличие от латинских Y/N, которые при ЙЦУКЕН превращаются в н/т).
        std::wcout << L"\nСохранить изменения? (Enter - да, Esc - нет): ";
        int c = _getch();
        if (c == 13) // Enter
        {
            if (SaveBindings(bindings))
            {
                std::wcout << L"Сохранено в " << kSettingsFileName << std::endl;
                return true;
            }
            std::wcout << L"Ошибка сохранения." << std::endl;
        }
        else // Esc (27) или любая другая клавиша
        {
            std::wcout << L"Изменения отменены." << std::endl;
        }
        return false;
    }
} // namespace Tetris
