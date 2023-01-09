//
//  imguiLayer.hpp
//  ImGuiFileDialog
//
//  Created by Juan Perez on 1/9/23.
//

#ifndef imguiLayer_hpp
#define imguiLayer_hpp

#include <stdio.h>

class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();
    
    void onAttach();
    void onDetach();
    void onEvent();
    
    void blockEvents(bool block) { m_blockEvents = block; }
    
    void begin();
    void end();
    
private:
    bool m_blockEvents = true;
};

#endif /* imguiLayer_hpp */
