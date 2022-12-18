#pragma once

#include <MetalKit/MetalKit.hpp>
#include "renderer.h"

class MTKViewDelegate : public MTK::ViewDelegate {
    public:
        MTKViewDelegate(MTL::Device* device);
        virtual ~MTKViewDelegate() override;
    
    private:
        std::unique_ptr<Renderer> m_renderer;
};
