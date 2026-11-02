cmake_minimum_required(VERSION 3.21)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(imgui-sdl3-image-webp-opengl-qt6-desktop CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -----------------------------------------------------------------------------
# Paths (Identical to your working SDL3 project)
# -----------------------------------------------------------------------------
if(WIN32)
    set(IMGUI_DIR         "C:/libs/imgui-1.93.0")
    set(GLAD_DIR          "C:/libs/glad-0.1.36/opengl-3.3")
    set(SDL3_DIR          "C:/libs/sdl3-3.4.18-mingw/lib/cmake/SDL3")
    set(SDL3_image_DIR    "C:/libs/sdl3_image-3.4.8-webp-mingw/lib/cmake/SDL3_image")
    set(webp_ROOT         "C:/libs/webp-1.6.0-mingw")
    set(webp_INCLUDE_DIR  "C:/libs/webp-1.6.0-mingw/include")
    set(webp_LIBRARY      "C:/libs/webp-1.6.0-mingw/lib/libwebp.a")
else()
    set(IMGUI_DIR         "$ENV{HOME}/libs/imgui-1.93.0")
    set(GLAD_DIR          "$ENV{HOME}/libs/glad-0.1.36/opengl-3.3")
    set(SDL3_DIR          "$ENV{HOME}/libs/sdl3-3.4.18-ubuntu/lib/cmake/SDL3")
    set(SDL3_image_DIR    "$ENV{HOME}/libs/sdl3_image-3.4.8-webp-ubuntu/lib/cmake/SDL3_image")
    set(webp_ROOT         "$ENV{HOME}/libs/webp-1.6.0-ubuntu")
    set(webp_INCLUDE_DIR  "$ENV{HOME}/libs/webp-1.6.0-ubuntu/include")
    set(webp_LIBRARY      "$ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebp.a")
endif()

# -----------------------------------------------------------------------------
# Locate Packages
# -----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

# Wayland is only present and required on Linux (Ubuntu / WSL)
if(UNIX AND NOT APPLE)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
endif()

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)

# -----------------------------------------------------------------------------
# Fix Ninja issue: Clear bogus relative link paths from SDL3_image static target
# -----------------------------------------------------------------------------
if(TARGET SDL3_image::SDL3_image-static)
    set_target_properties(SDL3_image::SDL3_image-static PROPERTIES INTERFACE_LINK_LIBRARIES "")
endif()

# -----------------------------------------------------------------------------
# ImGui Sources (OpenGL3 backend only)
# -----------------------------------------------------------------------------
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# -----------------------------------------------------------------------------
# Executable Target (qt_add_executable for static plugin support)
# -----------------------------------------------------------------------------
qt_add_executable(app
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    ${IMGUI_SOURCES}
    ${GLAD_DIR}/src/glad.c
)

# -----------------------------------------------------------------------------
# Include Directories & Definitions
# -----------------------------------------------------------------------------
target_include_directories(app PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLAD_DIR}/include
    ${webp_INCLUDE_DIR}
)

target_compile_definitions(app PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

# -----------------------------------------------------------------------------
# Copy Assets to Build Directory
# -----------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:app>/assets
    )
endif()

# -----------------------------------------------------------------------------
# Link Flags & Stripping (Applied to BOTH Windows & Ubuntu)
# -----------------------------------------------------------------------------
target_link_options(app PRIVATE 
    -static-libgcc 
    -static-libstdc++
    $<$<CONFIG:Release>:-s>
)

if(WIN32)
    target_link_libraries(app PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
        SDL3_image::SDL3_image-static
        SDL3::SDL3-static
        opengl32
        C:/libs/webp-1.6.0-mingw/lib/libwebpdemux.a
        C:/libs/webp-1.6.0-mingw/lib/libwebpmux.a
        C:/libs/webp-1.6.0-mingw/lib/libwebp.a
        C:/libs/webp-1.6.0-mingw/lib/libsharpyuv.a
    )

    # Full static linking and subsystem selection only on Windows
    target_link_options(app PRIVATE -static)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_options(app PRIVATE -mconsole)
    else()
        target_link_options(app PRIVATE -mwindows)
    endif()
else()
    target_link_libraries(app PRIVATE
        $ENV{HOME}/libs/qt-6.12.0-static-gcc/plugins/platforms/libqwayland.a
        Qt6::Widgets
        Qt6::OpenGLWidgets
        Qt6::WaylandClient
        SDL3_image::SDL3_image-static
        SDL3::SDL3-static
        GL
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebpdemux.a
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebpmux.a
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libwebp.a
        $ENV{HOME}/libs/webp-1.6.0-ubuntu/lib/libsharpyuv.a
    )
endif()
