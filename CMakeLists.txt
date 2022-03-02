cmake_minimum_required(VERSION 3.0)
project(UE4Console)

find_path(DETOURS_INCLUDE_DIRS detours.h PATH_SUFFIXES detours REQUIRED)
find_library(DETOURS_LIBRARIES detours REQUIRED)

set(UE4CONSOLE_SOURCES
    source/Main.cpp
    source/UConsole.cpp
    source/UGameViewportClient.cpp
    source/FOutputDeviceRedirector.cpp
)

set(UE4CONSOLE_LIBRARIES
    ${DETOURS_LIBRARIES}
    DbgHelp
)

set(UE4CONSOLE_INCLUDE_DIRS
    ${DETOURS_INCLUDE_DIRS}
)

add_library(winhttp SHARED source/WinHttp.cpp ${UE4CONSOLE_SOURCES})
target_link_libraries(winhttp PRIVATE ${UE4CONSOLE_LIBRARIES})
target_include_directories(winhttp PRIVATE ${UE4CONSOLE_INCLUDE_DIRS})

add_library(dsound SHARED source/DSound.cpp ${UE4CONSOLE_SOURCES})
target_link_libraries(dsound PRIVATE ${UE4CONSOLE_LIBRARIES})
target_include_directories(dsound PRIVATE ${UE4CONSOLE_INCLUDE_DIRS})
