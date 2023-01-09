//
//  imguiLayer.cpp
//  ImGuiFileDialog
//
//  Created by Juan Perez on 1/9/23.
//

#include "imguiLayer.hpp"

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_tables.cpp"
#include "imgui_impl_sdl.h"

#include "../ImGuiFileDialog/ImGuiFileDialog.h"

void ImGuiLayer::onAttach() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    ImGui::StyleColorsDark();
    
    // ImGui_ImplMetal_Init(device);
    // ImGui_ImplSDL2_InitForMetal(window);
}
