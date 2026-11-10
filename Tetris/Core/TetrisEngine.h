#pragma once

#include "Figure.h"
#include "GameData.h"

#include <string>
#include <utility>
#include <vector>

namespace Tetris
{
    // Действия игрока (агентский канал / MCP-тулзы).
    // Соответствуют тулзам: move_left, move_right, rotate_up, rotate_down, move_down, hard_drop.
    enum class PlayerAction
    {
        Left,
        Right,
        RotateUp,
        RotateDown,
        Down,
        HardDrop
    };

    // Снимок фигуры для передачи состояния (агентский канал).
    struct FigureSnapshot
    {
        int id{ -1 };
        int position{ -1 };
        int x{ 0 };
        int y{ 0 };
        int color{ 0 };
        bool inUse{ false };
    };

    // Снимок состояния партии (агентский канал): всё, что нужно, чтобы
    // воспроизвести поле и понять, что происходит.
    struct GameStateSnapshot
    {
        std::vector<std::vector<std::pair<int, int>>> stack;
        FigureSnapshot currentFigure;
        FigureSnapshot nextFigure;
        unsigned int score{ 0 };
        int level{ 1 };
        bool gameOver{ false };
        bool win{ false };
        bool showLevelUp{ false };
    };

    // Результат очистки линий (PrepareLineClear): для анимации стирания.
    struct LineClearInfo
    {
        bool lineCleared{ false };   // были ли очищены линии
        int clearRow{ -1 };          // стартовый ряд очистки
        unsigned int clearMask{ 0 }; // маска очищенных рядов (биты 3..0 — ряды)
    };

    // Результат перехода к новой фигуре (BeginFigure).
    struct StepResult
    {
        bool gameOver{ false }; // партия проиграна
        bool win{ false };      // партия выиграна
    };

    // Чистая игровая логика без I/O: состояние партии (стек, текущая/следующая
    // фигура, счёт, уровень, game over/победа) и методы продвижения.
    // Ни Sleep, ни рендер, ни ввод — только правила игры.
    //
    // Цикл партии (согласован с консольной обёрткой PlayGame):
    //   PrepareLineClear() — пометка полных рядов + очки (фаза Prepare).
    //   [рендер: анимация стирания, если lineCleared]
    //   CommitLineClear()  — сдвиг стека вниз (заполняет помеченные ряды).
    //   BeginFigure()      — обновление уровня, переход к новой фигуре,
    //                        проверка game over/победы.
    //   [рендер: кадр]
    //   FallOneStep()      — падение на один ряд (в цикле, пока не Locked).
    //   ApplyAction()      — действие игрока (LEFT/RIGHT/ROTATE/HARD_DROP).
    class TetrisEngine
    {
    private:
        std::vector<std::vector<std::pair<int, int>>> stack;
        Figure currentFigure;
        Figure nextFigure;
        unsigned int score{ 0 };
        int level{ 1 };
        bool gameOver{ false };
        bool win{ false };
        bool showLevelUp{ false };

        // Собирает снимок фигуры.
        static FigureSnapshot MakeFigureSnapshot(const Figure &figure);

    public:
        // Инициализирует состояние партии (пустой стек, счёт 0, уровень 1,
        // первая следующая фигура). Текущая фигура неактивна до первого
        // BeginFigure().
        void Reset();

        // Фаза очистки линий: находит первый полный ряд (с 3-го), помечает его
        // и до трёх полных рядов ниже (-1), начисляет очки. Возвращает
        // LineClearInfo (для анимации). Сдвиг стека вниз — в CommitLineClear().
        LineClearInfo PrepareLineClear();

        // Завершает очистку линий: сдвигает стек вниз (заполняет помеченные
        // ряды). Вызывается рендером после анимации стирания.
        void CommitLineClear();

        // Переход к новой фигуре: обновление уровня, текущая = следующая,
        // генерация новой следующей, проверка game over/победы.
        StepResult BeginFigure();

        // Падение текущего фигура на один ряд. fallLine — текущий ряд падения
        // (0..MAX_FALL_LINES-1). Locked — фигура зафиксирована в стеке.
        FallResult FallOneStep(int fallLine);

        // Применяет действие игрока к текущей фигуре. LEFT/RIGHT/ROTATE —
        // пробное движение (TryMove). DOWN — мягкий шаг вниз на один ряд
        // (агентский режим без гравитации), HARD_DROP — шаг падения
        // (FallOneStep): глубину (fallLine) управляет вызывающий. Возвращает
        // true при столкновении с полом/стеком (фигура зафиксирована в стеке);
        // падение на последнем ряду тоже фиксация (после него fallLine ==
        // MAX_FALL_LINES).
        bool ApplyAction(PlayerAction action, int fallLine);

        // Снимок состояния партии (агентский канал).
        GameStateSnapshot GetState() const;

        // Геттеры для рендера.
        const std::vector<std::vector<std::pair<int, int>>> &GetStack() const;
        const Figure &GetCurrentFigure() const;
        const Figure &GetNextFigure() const;
        unsigned int GetScore() const;
        int GetLevel() const;
        bool IsGameOver() const;
        bool IsWin() const;
        bool GetShowLevelUp() const;
    };
}
