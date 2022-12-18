#include "view_delegate.h"

MTKViewDelegate::MTKViewDelegate(MTL::Device* device) :
    MTK::ViewDelegate() {
        m_renderer = std::make_unique<Renderer>();
    }

MTKViewDelegate::~MTKViewDelegate() = default;

