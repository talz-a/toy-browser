add_executable(toy-browser 
    block_layout.cpp
    browser.cpp
    css_parser.cpp
    document_layout.cpp
    draw_commands.cpp
    html_parser.cpp
    main.cpp
    url.cpp
)

target_include_directories(toy-browser PRIVATE "${PROJECT_SOURCE_DIR}/include")

find_package(asio CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

target_link_libraries(toy-browser PRIVATE 
    asio::asio 
    OpenSSL::SSL 
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)

target_compile_definitions(toy-browser PRIVATE ASIO_STANDALONE)

if(MSVC)
    target_compile_options(toy-browser PRIVATE /W4 /permissive-)
else()
    target_compile_options(toy-browser PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_compile_definitions(toy-browser PRIVATE _WIN32_WINNT=0x0A00)
endif()
