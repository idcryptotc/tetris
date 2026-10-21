cmake_minimum_required(VERSION 3.21)

project(Tetris CXX)

# C++23 — стабильный уровень MSVC v145 (Visual Studio 2026).
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Архитектура задаётся при генерации: -A x64 (по умолчанию) или -A Win32 (x86).
add_executable(Tetris
    Tetris/Tetris.cpp
    Tetris/Main.cpp
    Tetris/Figure.cpp
    Tetris/Figure.h
    Tetris/Game.h
    Tetris/GameLoop.h
    Tetris/Keys.h
    Tetris/Settings.h
)

# Unicode-режим WinAPI (CharacterSet=Unicode в старом vcxproj).
# WIN32, _CONSOLE, _DEBUG/NDEBUG подставляет MSVC/CMake автоматически.
target_compile_definitions(Tetris PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(Tetris PRIVATE /W3)
endif()
