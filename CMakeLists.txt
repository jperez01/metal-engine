cmake_minimum_required(VERSION 3.20)
project (metal_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(SDL2 REQUIRED COMPONENTS SDL2)

add_subdirectory(metal-cmake)

add_subdirectory(third-party)

file(GLOB SOURCES
    src/*.h
    src/*.cpp
    src/utility/*.h
    src/utility/*.hpp
    src/utility/*.cpp
)

add_executable(metal_engine
    ${SOURCES}
)

set(assimp "${CMAKE_SOURCE_DIR}/third-party/lib/libassimpd.5.2.4.dylib")

target_include_directories(metal_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party")

target_link_libraries(metal_engine
    METAL_CPP imgui SDL2::SDL2 ${assimp} stb glm ImGuiFileDialog
)
