#include "metal-cmake/definition.cpp"
#include "renderer.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_metal.h"

#include <SDL2/SDL.h>

float lastX = 800.0f / 2.0;
float lastY = 600.0f / 2.0;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main(int argc, char* argv[]) {
    auto myRenderer = Renderer();
    myRenderer.run();

    return 0;
}
