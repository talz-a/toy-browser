add_executable(toy-browser)
add_executable(toy::browser ALIAS toy-browser)

target_sources(toy-browser
    PRIVATE
        block_layout.cpp
        browser.cpp
        css_parser.cpp
        document_layout.cpp
        draw_commands.cpp
        html_parser.cpp
        main.cpp
        url.cpp
)

target_sources(toy-browser
    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
        FILES
            "${PROJECT_SOURCE_DIR}/include/browser/block_layout.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/browser.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/constants.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/css_parser.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/document_layout.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/draw_commands.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/html_parser.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/url.hpp"
            "${PROJECT_SOURCE_DIR}/include/browser/utils.hpp"
)

find_package(asio REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SFML REQUIRED COMPONENTS Graphics)

target_link_libraries(toy-browser 
    PRIVATE 
        asio::asio 
        OpenSSL::SSL 
        SFML::Graphics
)

target_compile_definitions(toy-browser PRIVATE ASIO_STANDALONE)

set_target_properties(toy-browser PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
