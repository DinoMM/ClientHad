cmake_minimum_required(VERSION 3.18)
project(ClientHad)

set(CMAKE_CXX_STANDARD 11)

add_executable(ClientHad main.cpp
        Snake.cpp
        Snake.h)
target_link_libraries(ClientHad pthread)

# Link the ncurses library
target_link_libraries(ClientHad ${CURSES_LIBRARIES} pthread)

# Include the ncurses headers
target_include_directories(ClientHad PRIVATE ${CURSES_INCLUDE_DIR})

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

target_link_libraries(ClientHad ${CURSES_LIBRARIES})
