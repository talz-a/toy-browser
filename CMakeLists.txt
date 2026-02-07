cmake_minimum_required(VERSION 3.20)
project(toy-browser LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CLANG_TIDY_EXE clang-tidy)
if(CLANG_TIDY_EXE)
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
endif()

find_package(asio CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System)

add_executable(toy-browser 
    src/main.cpp 
    src/url.cpp
    src/browser.cpp
    src/block_layout.cpp
    src/html_parser.cpp
    src/document_layout.cpp
    src/draw_commands.cpp
    src/css_parser.cpp
)

target_include_directories(toy-browser PRIVATE include)

target_link_libraries(toy-browser PRIVATE 
    asio::asio 
    OpenSSL::SSL 
    SFML::Graphics
    SFML::Window
    SFML::System
)

target_compile_definitions(toy-browser PRIVATE ASIO_STANDALONE)
