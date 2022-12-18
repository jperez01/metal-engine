#pragma once
#include <Metal/Metal.hpp>
#include <AppKit/NSApplication.hpp>

#include "view_delegate.h"

class AppDelegate : public NS::ApplicationDelegate {
    public:
        ~AppDelegate();

        NS::Menu* createMenuBar();

        virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
        virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
        virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;

    private:
        NS::Window* m_window;
        MTK::View* m_view;
        MTL::Device* m_device;
        std::unique_ptr<MTKViewDelegate> m_viewDelegate;
};