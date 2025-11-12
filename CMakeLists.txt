cmake_minimum_required(VERSION 3.16)
project(Bookeeper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets Charts)

set(PROJECT_SOURCES
    src/main.cpp
    src/core/Entities.cpp
    src/core/JsonStorage.cpp
    src/core/LedgerService.cpp
    src/ui/LoginWindow.cpp
    src/ui/BillEditorDialog.cpp
    src/ui/ReminderDialog.cpp
    src/ui/MainWindow.cpp
)

add_executable(bookeeper ${PROJECT_SOURCES})

target_include_directories(bookeeper PRIVATE src)

if(MSVC)
    target_compile_options(bookeeper PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(bookeeper PRIVATE -Wall -Wextra -pedantic)
endif()

target_link_libraries(bookeeper PRIVATE Qt5::Widgets Qt5::Charts)
