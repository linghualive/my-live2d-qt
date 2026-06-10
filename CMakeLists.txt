cmake_minimum_required(VERSION 3.16)
project(QDesktopPet VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Platform detection ---
include(cmake/PlatformDetection.cmake)

# --- Qt: prefer Qt6, fallback to Qt5 ---
set(QT_MAJOR 0)
find_package(Qt6 QUIET COMPONENTS Core Gui Widgets OpenGLWidgets)
if(Qt6_FOUND)
    set(QT_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
    set(QT_MAJOR 5)
    message(STATUS "Using Qt5 (fallback)")
    if(PLATFORM_X11)
        find_package(Qt5 REQUIRED COMPONENTS X11Extras)
    endif()
endif()

find_package(OpenGL REQUIRED)

# --- CubismNativeFramework sources (OpenGL renderer only) ---
file(GLOB_RECURSE CUBISM_FRAMEWORK_SOURCES
    third_party/CubismNativeFramework/src/*.cpp
)
# Remove non-OpenGL rendering backends
list(FILTER CUBISM_FRAMEWORK_SOURCES EXCLUDE REGEX ".*/Rendering/D3D9/.*")
list(FILTER CUBISM_FRAMEWORK_SOURCES EXCLUDE REGEX ".*/Rendering/D3D11/.*")
list(FILTER CUBISM_FRAMEWORK_SOURCES EXCLUDE REGEX ".*/Rendering/Vulkan/.*")
list(FILTER CUBISM_FRAMEWORK_SOURCES EXCLUDE REGEX ".*/Rendering/Metal/.*")

# --- QtLive2d widget sources ---
file(GLOB QTLIVE2D_SOURCES
    third_party/QtLive2d/*.cpp
    third_party/QtLive2d/*.hpp
)

# --- App sources ---
set(CORE_SOURCES
    src/core/ProcessLock.h
    src/core/ProcessLock.cpp
    src/core/Configuration.h
    src/core/Configuration.cpp
    src/core/ModelInfo.h
    src/core/ModelManager.h
    src/core/ModelManager.cpp
)

set(UI_SOURCES
    src/ui/PetWindow.h
    src/ui/PetWindow.cpp
    src/ui/TrayIconManager.h
    src/ui/TrayIconManager.cpp
    src/ui/PreferencesDialog.h
    src/ui/PreferencesDialog.cpp
)

set(PLATFORM_SOURCES
    src/platform/IMouseTracker.h
    src/platform/IInputPassthrough.h
    src/platform/PlatformFactory.h
    src/platform/PlatformFactory.cpp
)

if(PLATFORM_X11)
    list(APPEND PLATFORM_SOURCES
        src/platform/x11/X11MouseTracker.h
        src/platform/x11/X11MouseTracker.cpp
        src/platform/x11/X11InputPassthrough.h
        src/platform/x11/X11InputPassthrough.cpp
    )
endif()

if(PLATFORM_WAYLAND)
    list(APPEND PLATFORM_SOURCES
        src/platform/wayland/WaylandMouseTracker.h
        src/platform/wayland/WaylandMouseTracker.cpp
        src/platform/wayland/WaylandInputPassthrough.h
        src/platform/wayland/WaylandInputPassthrough.cpp
    )
endif()

if(PLATFORM_MACOS)
    enable_language(OBJCXX)
    list(APPEND PLATFORM_SOURCES
        src/platform/macos/MacMouseTracker.h
        src/platform/macos/MacMouseTracker.mm
        src/platform/macos/MacInputPassthrough.h
        src/platform/macos/MacInputPassthrough.mm
        src/platform/macos/MacAutoStart.mm
        src/platform/macos/MacWindowHelper.h
        src/platform/macos/MacWindowHelper.mm
    )
elseif(PLATFORM_WINDOWS)
    list(APPEND PLATFORM_SOURCES
        src/platform/windows/WinMouseTracker.h
        src/platform/windows/WinMouseTracker.cpp
        src/platform/windows/WinInputPassthrough.h
        src/platform/windows/WinInputPassthrough.cpp
        src/platform/windows/WinAutoStart.cpp
    )
else()
    list(APPEND PLATFORM_SOURCES
        src/platform/AutoStartStub.cpp
    )
endif()

# --- Executable ---
add_executable(QDesktopPet
    src/main.cpp
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UI_SOURCES}
    ${CUBISM_FRAMEWORK_SOURCES}
    ${QTLIVE2D_SOURCES}
)

target_include_directories(QDesktopPet PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/QtLive2d
    ${CMAKE_SOURCE_DIR}/third_party/CubismNativeFramework/src
    ${CMAKE_SOURCE_DIR}/third_party/CubismCore/include
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

# --- CubismCore static library ---
if(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(CUBISM_CORE_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/CubismCore/lib/macos/arm64")
    else()
        set(CUBISM_CORE_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/CubismCore/lib/macos/x86_64")
    endif()
elseif(WIN32)
    set(CUBISM_CORE_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/CubismCore/lib/windows/x86_64")
elseif(UNIX)
    set(CUBISM_CORE_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/CubismCore/lib/linux/${CMAKE_SYSTEM_PROCESSOR}")
endif()

find_library(CUBISM_CORE_LIB
    NAMES Live2DCubismCore
    PATHS "${CUBISM_CORE_LIB_DIR}"
    NO_DEFAULT_PATH
)

if(CUBISM_CORE_LIB)
    message(STATUS "CubismCore found: ${CUBISM_CORE_LIB}")
else()
    message(FATAL_ERROR "CubismCore not found in ${CUBISM_CORE_LIB_DIR}")
endif()

# --- Linking ---
if(QT_MAJOR EQUAL 6)
    target_link_libraries(QDesktopPet PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
        OpenGL::GL
        ${CUBISM_CORE_LIB}
    )
else()
    target_link_libraries(QDesktopPet PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::OpenGL
        OpenGL::GL
        ${CUBISM_CORE_LIB}
    )
    if(PLATFORM_X11)
        target_link_libraries(QDesktopPet PRIVATE Qt5::X11Extras)
    endif()
    target_compile_definitions(QDesktopPet PRIVATE QT5_COMPAT)
endif()

# Platform-specific linking
if(PLATFORM_X11)
    target_link_libraries(QDesktopPet PRIVATE
        X11::X11
        X11::Xext
    )
    if(X11_Xtst_FOUND)
        target_link_libraries(QDesktopPet PRIVATE X11::Xtst)
    endif()
    target_compile_definitions(QDesktopPet PRIVATE HAS_X11)
endif()

if(PLATFORM_WAYLAND)
    target_link_libraries(QDesktopPet PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
    target_include_directories(QDesktopPet PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_compile_definitions(QDesktopPet PRIVATE HAS_WAYLAND)
endif()

if(PLATFORM_MACOS)
    target_link_libraries(QDesktopPet PRIVATE
        "-framework Cocoa"
        "-framework CoreGraphics"
        "-framework IOKit"
    )
    target_compile_definitions(QDesktopPet PRIVATE HAS_MACOS)
endif()

if(PLATFORM_WINDOWS)
    target_link_libraries(QDesktopPet PRIVATE user32 advapi32 opengl32)
    target_compile_definitions(QDesktopPet PRIVATE HAS_WINDOWS)
    set_target_properties(QDesktopPet PROPERTIES WIN32_EXECUTABLE TRUE)
    add_custom_command(TARGET QDesktopPet POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CUBISM_CORE_LIB_DIR}/Live2DCubismCore.dll"
            "$<TARGET_FILE_DIR:QDesktopPet>/Live2DCubismCore.dll"
        COMMENT "Copying Live2DCubismCore.dll..."
    )
endif()

# Suppress OpenGL deprecation warnings on macOS
if(APPLE)
    target_compile_definitions(QDesktopPet PRIVATE GL_SILENCE_DEPRECATION)
endif()

# --- macOS .app bundle ---
if(APPLE)
    set_target_properties(QDesktopPet PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    )
endif()

# --- Copy Resources (if present) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/Resources")
    if(APPLE)
        add_custom_command(TARGET QDesktopPet POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/Resources"
                "$<TARGET_BUNDLE_CONTENT_DIR:QDesktopPet>/Resources"
            COMMENT "Copying model resources into app bundle..."
        )
    else()
        add_custom_command(TARGET QDesktopPet POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/Resources"
                "$<TARGET_FILE_DIR:QDesktopPet>/Resources"
            COMMENT "Copying bundled model resources..."
        )
    endif()
endif()

# --- macOS: macdeployqt target ---
if(APPLE)
    find_program(MACDEPLOYQT macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT)
        add_custom_target(deploy
            COMMAND "${MACDEPLOYQT}" "$<TARGET_BUNDLE_DIR:QDesktopPet>"
            DEPENDS QDesktopPet
            COMMENT "Running macdeployqt to bundle Qt frameworks..."
        )
    endif()
endif()
