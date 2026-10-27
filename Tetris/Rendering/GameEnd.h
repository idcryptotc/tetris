#pragma once

namespace Tetris
{
    // Экраны конца партии: пиксельное сообщение (5x7-глифы, масштаб 1),
    // счёт/уровень и подсказка «Для выхода в меню нажми Enter».
    // Truecolor в Adaptive, 16-цвет в Fixed. Оба висят до нажатия Enter
    // (Enter в игре не участвует — нажатия из «отчаянной битвы» его не
    // закрывают, а буфер клавиатуры перед ожиданием сбрасывается).

    // Экран проигрыша: «GAME OVER». Показывается при переполнении стека.
    void ShowGameOver(unsigned int score, int level);

    // Экран победы: «YOU WIN». Показывается при достижении MAX_LEVEL.
    void ShowWin(unsigned int score, int level);
}
