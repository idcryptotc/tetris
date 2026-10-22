#pragma once

namespace Tetris
{
    // Генерация фигур: случайный id и стартовая позиция.

    // Случайный id фигуры из таблицы FIGURES.
    int RandomFigureId();

    // Случайная стартовая позиция фигуры в зависимости от её типа (-1 — нет позиции).
    int PositionById(int figureId);
}
