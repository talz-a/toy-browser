# Toy Browser

A lightweight web browser engine implemented in C++, following the architectural principles of "Web Browser Engineering". This project demonstrates the core components of a modern browser, including networking, parsing, layout, and hardware-accelerated rendering.

## Technical Highlights

- **Networking Stack**: Implemented a multi-protocol request system using Asio. Supports both HTTP and HTTPS (via OpenSSL).
- **Custom Parsers**: Built recursive-descent parsers for HTML and CSS. The CSS engine supports tag selectors, descendant combinators, and property inheritance.
- **Layout Engine**: Features a block layout system that handles box model calculations, line breaking, and baseline-aligned text rendering using font metrics.
- **Graphics**: Utilizes SDL3 and SDL3_ttf for hardware-accelerated 2D rendering and high-quality text rasterization.

## Dependencies

The project uses **vcpkg** in manifest mode to manage the following dependencies:
- SDL3
- SDL3_ttf
- Asio
- OpenSSL

## Building the Project

Ensure you have CMake 3.25+ and a C++23 compatible compiler installed.

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

The executable will be located in the build directory.