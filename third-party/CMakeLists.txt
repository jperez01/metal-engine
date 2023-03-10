add_library(imgui STATIC)
add_library(ImGuiFileDialog STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
    imgui/imgui.h
    imgui/imgui.cpp
    imgui/imconfig.h
    
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    
    imgui/imgui_impl_metal.mm
    imgui/imgui_impl_sdl.h
    imgui/imgui_impl_sdl.cpp)
    

target_include_directories(ImGuiFileDialog PUBLIC ImGuiFileDialog)

target_sources(ImGuiFileDialog PRIVATE
    ImGuiFileDialog/ImGuiFileDialog.h
    ImGuiFileDialog/ImGuiFileDialog.cpp)
    
target_link_libraries(ImGuiFileDialog PUBLIC imgui)

file(GLOB ASSIMP_SOURCES
    assimp/*.h
    assimp/*.hpp
)

add_library(assimp INTERFACE)
target_include_directories(assimp INTERFACE .)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE .)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

target_link_libraries(imgui PUBLIC METAL_CPP SDL2::SDL2)
