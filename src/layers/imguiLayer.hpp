//
//  imguiLayer.hpp
//  ImGuiFileDialog
//
//  Created by Juan Perez on 1/9/23.
//

#ifndef imguiLayer_hpp
#define imguiLayer_hpp

#include <stdio.h>
#include <Metal/Metal.hpp>
#include <SDL2/SDL.h>

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_impl_sdl.h"

#include "ImGuiFileDialog/ImGuiFileDialog.h"

class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();
    
    void onAttach(MTL::Device* device, SDL_Window* window);
    void onDetach();
    void onEvent();
    
    void blockEvents(bool block) { m_blockEvents = block; }
    
    void begin(MTL::RenderPassDescriptor* descriptor);
    void end(MTL::CommandBuffer* cmd, MTL::RenderCommandEncoder* encoder);
    
private:
    bool m_blockEvents = true;
};

#endif /* imguiLayer_hpp */
