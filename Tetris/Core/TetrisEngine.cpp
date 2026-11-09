#include "TetrisEngine.h"

#include "FigureGenerator.h"
#include "Game.h"
#include "GameData.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Tetris
{
    // Инициализирует состояние партии (пустой стек, счёт 0, уровень 1,
    // первая следующая фигура). Текущая фигура неактивна до первого
    // BeginFigure().
    void TetrisEngine::Reset()
    {
        stack.assign(HEIGHT, std::vector<std::pair<int, int>>(WIDTH + 1, { 0, 0 }));
        score = 0;
        level = 1;
        gameOver = false;
        win = false;
        showLevelUp = false;
        currentFigure.SetUse(false);

        int figureId = RandomFigureId();
        int figurePosition = PositionById(figureId);
        nextFigure.SetBasicParameters(figureId, figurePosition);
    }

    // Фаза очистки линий: находит первый полный ряд (с 3-го), помечает его
    // и до трёх полных рядов ниже (-1), начисляет очки. Возвращает
    // LineClearInfo (для анимации). Сдвиг стека вниз — в CommitLineClear().
    LineClearInfo TetrisEngine::PrepareLineClear()
    {
        LineClearInfo info;
        std::size_t gameStackSize = stack.size();

        for (std::size_t i = 3; i < gameStackSize; ++i)
        {
            if (stack[i][WIDTH].first == 10)
            {
                unsigned int mask = 0x8;
                stack[i][WIDTH].first = -1;

                if (i + 1 < HEIGHT)
                {
                    mask += stack[i + 1][WIDTH].first == 10 ? (stack[i + 1][WIDTH].first = -1, 0x4) : 0x0;
                }

                if (i + 2 < HEIGHT)
                {
                    mask += stack[i + 2][WIDTH].first == 10 ? (stack[i + 2][WIDTH].first = -1, 0x2) : 0x0;
                }

                if (i + 3 < HEIGHT)
                {
                    mask += stack[i + 3][WIDTH].first == 10 ? (stack[i + 3][WIDTH].first = -1, 0x1) : 0x0;
                }

                // Кумулятивный подсчёт очков:
                // чем больше линий очищено, тем выше базовый счёт:
                // 4 линии = 10 очков, 3 линии = 6, 2 линии = 3, 1 линия = 1;
                // результат умножается на текущий уровень.
                // Таблица: индекс = количество очищенных линий.
                const unsigned int LINES_SCORE_TABLE[5] = { 0, 1, 3, 6, 10 };
                unsigned int clearedLines = ((mask & 0x8) >> 3) + ((mask & 0x4) >> 2) + ((mask & 0x2) >> 1) + (mask & 1);
                score += LINES_SCORE_TABLE[clearedLines] * static_cast<unsigned int>(level);

                info.lineCleared = true;
                info.clearRow = static_cast<int>(i);
                info.clearMask = mask;
                break;
            }
        }

        return info;
    }

    // Завершает очистку линий: сдвигает стек вниз (заполняет помеченные
    // ряды). Вызывается рендером после анимации стирания.
    void TetrisEngine::CommitLineClear()
    {
        std::size_t gameStackSize = stack.size();
        std::size_t gameStackISize = stack[0].size() - 1;

        for (std::size_t k = gameStackSize - 1, t = k; k > 0 && t > 0; --t)
        {
            if (stack[t][WIDTH].first == -1)
            {
                continue;
            }

            for (std::size_t j = 0; j < gameStackISize + 1; ++j)
            {
                stack[k][j] = stack[t][j];
            }

            --k;
        }
    }

    // Переход к новой фигуре: обновление уровня, текущая = следующая,
    // генерация новой следующей, проверка game over/победы.
    StepResult TetrisEngine::BeginFigure()
    {
        // Уровень ограничен MAX_LEVEL: иначе LEVEL_TIME_STEP[level - 1]
        // (25 элементов, индексы 0..24) при level = 26 — выход за
        // границы (OOB). Победа проверяется ниже.
        const int prevLevel = level;
        CheckScoreForLevel(score, level);
        if (level > MAX_LEVEL)
        {
            level = MAX_LEVEL;
        }

        // Вспышка level up: показывается в течение раунда, в котором
        // поднят уровень (оверлей поверх кадра, без паузы).
        showLevelUp = level > prevLevel;

        currentFigure = nextFigure;
        currentFigure.SetUse(true);

        int figureId = RandomFigureId();
        int figurePosition = PositionById(figureId);
        nextFigure.SetBasicParameters(figureId, figurePosition);

        StepResult result;

        // Победа проверяется после game over, т.к. переполнение стека
        // имеет приоритет (партия проиграна).
        if (CheckGameOver(stack))
        {
            gameOver = true;
            result.gameOver = true;
        }
        else if (level >= MAX_LEVEL)
        {
            win = true;
            result.win = true;
        }

        return result;
    }

    // Падение текущего фигура на один ряд. Locked — фигура зафиксирована
    // в стеке (столкновение или достигнут последний ряд).
    FallResult TetrisEngine::FallOneStep(int fallLine)
    {
        return TryFallOneStep(currentFigure, stack, fallLine);
    }

    // Применяет действие игрока к текущей фигуре. LEFT/RIGHT/ROTATE —
    // пробное движение (TryMove). HARD_DROP — падение на один ряд
    // (FallOneStep). Возвращает true, если фигура зафиксирована
    // (только для HARD_DROP).
    bool TetrisEngine::ApplyAction(PlayerAction action, int fallLine)
    {
        switch (action)
        {
            case PlayerAction::Left:
                TryMove(currentFigure, stack, MoveDirection::Left, Actions::LEFT);
                return false;
            case PlayerAction::Right:
                TryMove(currentFigure, stack, MoveDirection::Right, Actions::RIGHT);
                return false;
            case PlayerAction::RotateUp:
                TryMove(currentFigure, stack, MoveDirection::RotateUp, Actions::ROTATE);
                return false;
            case PlayerAction::RotateDown:
                TryMove(currentFigure, stack, MoveDirection::RotateDown, Actions::ROTATE);
                return false;
            case PlayerAction::HardDrop:
                return FallOneStep(fallLine) == FallResult::Locked;
        }

        return false;
    }

    // Собирает снимок фигуры.
    FigureSnapshot TetrisEngine::MakeFigureSnapshot(const Figure &figure)
    {
        FigureSnapshot snap;
        snap.id = figure.GetId();
        snap.position = figure.GetPosition();
        snap.x = figure.GetPositionX();
        snap.y = figure.GetPositionY();
        snap.color = figure.GetColorAsInt();
        snap.inUse = figure.GetUse();
        return snap;
    }

    // Снимок состояния партии (агентский канал).
    GameStateSnapshot TetrisEngine::GetState() const
    {
        GameStateSnapshot snap;
        snap.stack = stack;
        snap.currentFigure = MakeFigureSnapshot(currentFigure);
        snap.nextFigure = MakeFigureSnapshot(nextFigure);
        snap.score = score;
        snap.level = level;
        snap.gameOver = gameOver;
        snap.win = win;
        snap.showLevelUp = showLevelUp;
        return snap;
    }

    const std::vector<std::vector<std::pair<int, int>>> &TetrisEngine::GetStack() const
    {
        return stack;
    }

    const Figure &TetrisEngine::GetCurrentFigure() const
    {
        return currentFigure;
    }

    const Figure &TetrisEngine::GetNextFigure() const
    {
        return nextFigure;
    }

    unsigned int TetrisEngine::GetScore() const
    {
        return score;
    }

    int TetrisEngine::GetLevel() const
    {
        return level;
    }

    bool TetrisEngine::IsGameOver() const
    {
        return gameOver;
    }

    bool TetrisEngine::IsWin() const
    {
        return win;
    }

    bool TetrisEngine::GetShowLevelUp() const
    {
        return showLevelUp;
    }
}
