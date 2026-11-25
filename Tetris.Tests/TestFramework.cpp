#include "TestFramework.h"

#include <cstddef>
#include <cstdio>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <io.h>
#include <windows.h>

namespace TetrisTest
{
    // Зарегистрированные тесты: порядок — порядок статических инициализаций
    // (между модулями произвольный; тесты от порядка не зависят).
    static std::vector<TestCase> g_tests;

    // Число упавших проверок в текущем тесте: сбрасывается перед каждым тестом в RunAll.
    static int g_currentFailures = 0;

    void RegisterTestCase(const TestCase &test)
    {
        g_tests.push_back(test);
    }

    // Узкий путь из __FILE__ (UTF-8, /utf-8) в UTF-16: отчёт не зависит
    // от кодовой страницы системы.
    static std::wstring ToWide(const std::string &text)
    {
        if (text.empty())
        {
            return std::wstring();
        }

        int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
            static_cast<int>(text.size()), nullptr, 0);
        std::wstring result(static_cast<std::size_t>(size), L'\0');
        // Число записанных символов — то, что просили, результат не нужен.
        (void)MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
            static_cast<int>(text.size()), &result[0], size);
        return result;
    }

    void Check(bool condition, const char *expression, const char *file, int line)
    {
        if (condition)
        {
            return;
        }

        ++g_currentFailures;
        std::wcout << L"    СБОЙ: " << ToWide(expression)
            << L" (" << ToWide(file) << L", строка " << line << L")\n";
    }

    int RunAll()
    {
        // Консоль в unicode-режиме (то же правило, что в игре): отчёт — на русском.
        // Режим уже задан, возвращаемый код не нужен.
        (void)_setmode(_fileno(stdout), _O_U16TEXT);

        int passed = 0;
        int failed = 0;

        for (const auto &test : g_tests)
        {
            g_currentFailures = 0;
            std::wcout << ToWide(test.name) << L"...\n";

            try
            {
                test.run();
            }
            catch (const std::exception &e)
            {
                // Неотловленное исключение в тесте — сбой этого теста,
                // а не сбой всего прогона.
                ++g_currentFailures;
                std::wcout << L"    ИСКЛЮЧЕНИЕ: " << ToWide(e.what()) << L"\n";
            }

            if (g_currentFailures == 0)
            {
                ++passed;
                std::wcout << L"  OK\n";
            }
            else
            {
                ++failed;
                std::wcout << L"  FAIL (" << g_currentFailures << L")\n";
            }
        }

        std::wcout << L"\nИтог: " << passed << L" прошло, " << failed << L" упало\n";
        return failed == 0 ? 0 : 1;
    }
}

// Точка входа тестового проекта: прогон всех зарегистрированных тестов.
// Код возврата: 0 — все прошли, 1 — есть падения.
int main()
{
    return TetrisTest::RunAll();
}
