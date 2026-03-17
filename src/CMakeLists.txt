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

find_package(asio REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SFML REQUIRED COMPONENTS Graphics)

target_link_libraries(toy-browser PRIVATE 
    asio::asio 
    OpenSSL::SSL 
    SFML::Graphics
)

target_compile_definitions(toy-browser PRIVATE ASIO_STANDALONE)
