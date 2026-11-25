#pragma once

#include <string>

namespace TetrisTest
{
    // Тестовый случай: имя и функция-тело.
    struct TestCase
    {
        std::string name;
        void (*run)();
    };

    // Регистрация тестового случая: вызывается макросом TEST_CASE
    // статической инициализацией до main() (порядок регистрации между
    // модулями не гарантирован и не требуется).
    void RegisterTestCase(const TestCase &test);

    // Регистрация проверки: при сбое — выражение, файл и строка.
    void Check(bool condition, const char *expression, const char *file, int line);

    // Сравнение двух значений одного типа (макрос CHECK_EQ): при различии
    // — сбой через Check.
    template <typename T>
    void CheckEq(const T &expected, const T &actual, const char *expression,
        const char *file, int line)
    {
        if (expected != actual)
        {
            Check(false, expression, file, line);
        }
    }

    // Прогон всех зарегистрированных тестов: печатает отчёт
    // «пройдено/упало» и код возврата (0 — все прошли).
    int RunAll();
}

// Объявление тестового случая: имя — идентификатор PascalCase,
// тело — сразу за макросом в том же модуле.
#define TEST_CASE(name) \
    static void TestBody_##name(); \
    static const bool testRegistered_##name = [] \
    { \
        TetrisTest::RegisterTestCase(TetrisTest::TestCase{ #name, TestBody_##name }); \
        return true; \
    }(); \
    static void TestBody_##name()

// Проверка условия: при сбое — файл и строка, тест выполняется дальше.
#define CHECK(condition) \
    TetrisTest::Check((condition), #condition, __FILE__, __LINE__)

// Сравнение «ожидается / фактически» (значения одного типа):
// при различии — файл и строка.
#define CHECK_EQ(expected, actual) \
    TetrisTest::CheckEq((expected), (actual), #expected " == " #actual, __FILE__, __LINE__)
