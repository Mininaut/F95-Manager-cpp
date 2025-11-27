cmake_minimum_required(VERSION 3.16)
project(F95ManagerCpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(f95_manager_cpp
    src/main.cpp
    src/app/app.cpp
    src/logger.cpp
)
set_target_properties(f95_manager_cpp PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(f95_manager_cpp PRIVATE src ${PROJECT_SOURCE_DIR}/vendor)
target_compile_definitions(f95_manager_cpp PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
target_link_libraries(f95_manager_cpp PRIVATE winhttp shell32 ole32 user32 gdi32)

# ImGui GUI target
add_executable(f95_manager_gui
    src/gui/imgui_main.cpp
    src/logger.cpp
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/backends/imgui_impl_win32.cpp
    vendor/imgui/backends/imgui_impl_dx11.cpp
)
set_target_properties(f95_manager_gui PROPERTIES WIN32_EXECUTABLE TRUE)
target_include_directories(f95_manager_gui PRIVATE src vendor vendor/imgui vendor/imgui/backends)
target_compile_definitions(f95_manager_gui PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
# MinGW needs -municode for wWinMain entry point
target_link_options(f95_manager_gui PRIVATE -municode)
target_link_libraries(f95_manager_gui PRIVATE d3d11 dxgi d3dcompiler user32 gdi32 shell32 ole32 imm32 dwmapi winhttp)
