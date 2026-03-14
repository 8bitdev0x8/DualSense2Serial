cmake_minimum_required(VERSION 3.23)
project(DualSense2Serial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # Link the MSVC runtime statically to reduce external runtime dependencies.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Suppress deprecation warnings from vendored third-party CMake projects.
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Build SDL as a static library to avoid requiring SDL2.dll at runtime.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.11
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(SDL2 imgui nlohmann_json)

add_library(imgui_lib
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC SDL2::SDL2-static)

add_executable(DualSense2Serial
  src/main.cpp
  src/App.cpp
  src/ControllerInput.cpp
  src/SerialPort.cpp
  src/Config.cpp
)

target_include_directories(DualSense2Serial PRIVATE include)

target_link_libraries(DualSense2Serial PRIVATE
  SDL2::SDL2-static
  imgui_lib
  nlohmann_json::nlohmann_json
)

if(WIN32)
  target_compile_definitions(DualSense2Serial PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX SDL_MAIN_HANDLED)
endif()
