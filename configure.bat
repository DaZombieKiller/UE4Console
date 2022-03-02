@echo off
set CMAKE_TOOLCHAIN_FILE=%UserProfile%\source\repos\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" -G "Visual Studio 17 2022" -A x64 -B build 
