cmake_minimum_required(VERSION 3.20)
project(toy-browser LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(asio CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(toy-browser 
    src/main.cpp 
    src/url.cpp
)

target_include_directories(toy-browser PRIVATE include)

target_link_libraries(toy-browser PRIVATE 
    asio::asio 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

target_compile_definitions(toy-browser PRIVATE ASIO_STANDALONE)