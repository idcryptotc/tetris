#pragma once

namespace Tetris
{
    // Экраны конца партии: пиксельное сообщение (5x7-глифы, масштаб 1),
    // счёт/уровень и подсказка «Для выхода в меню нажми Enter».
    // Truecolor в Adaptive, 16-цвет в Fixed. Оба висят до нажатия Enter
    // (Enter в игре не участвует — нажатия из «отчаянной битвы» его не
    // закрывают, а буфер клавиатуры перед ожиданием сбрасывается).

    // Экран проигрыша: «GAME OVER». Показывается при переполнении стека.
    // waitForEnter — ждать Enter (true, по умолчанию); в агентском режиме
    // false — экран показывается и сразу возвращается (ожидание restart).
    void ShowGameOver(unsigned int score, int level, bool waitForEnter = true);

    // Экран победы: «YOU WIN». Показывается при достижении MAX_LEVEL.
    // waitForEnter — как в ShowGameOver.
    void ShowWin(unsigned int score, int level, bool waitForEnter = true);
}
