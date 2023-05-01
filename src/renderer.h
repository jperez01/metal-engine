#pragma once

#include <vector>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <AppKit/AppKit.hpp>
#include <SDL2/SDL.h>

#include "utility/model.hpp"
#include "utility/camera.hpp"
#include "utility/gizmo.hpp"

#include "layers/imguiLayer.hpp"

static constexpr size_t kInstanceRows = 10;
static constexpr size_t kInstanceColumns = 10;
static constexpr size_t kInstanceDepth = 10;
static constexpr size_t kNumInstances = kInstanceRows * kInstanceColumns * kInstanceDepth;
static constexpr size_t kMaxFramesInFlight = 3;

static constexpr uint32_t kTextureWidth = 128;
static constexpr uint32_t kTextureHeight = 128;

struct cameraInfo {
    float lastX = 1280.0f / 2.0;
    float lastY = 720.0f / 2.0;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    bool firstMouse = true;
};

struct pointLight {
    float position[3];
    float ambient[3];
    float specular[3];
    float diffuse[3];
};

struct directionalLight {
    float direction[3];
    float ambient[3];
    float specular[3];
    float diffuse[3];
};


class Renderer {
public:
    Renderer();
    ~Renderer();

    void buildShaders();
    void buildComputePipeline();
    void buildDepthStencilStates();
    void buildBuffers();
    void buildTextures();
    void buildFrameData();
    
    void asyncImportModel(std::string path);

    void createLights();
    void buildCubemap();

    void draw(CA::MetalDrawable* drawable);
    void drawModelOnly(CA::MetalDrawable* drawable);

    void initWindow();
    void run();

    void generateMandelbrotTexture(MTL::CommandBuffer* cmd);
    
    Camera m_camera;
    
private:
    cameraInfo m_cameraInfo;
    CA::MetalLayer* m_layer;
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;

    MTL::Device* m_device;
    MTL::CommandQueue* m_commandQueue;

    MTL::Library* m_shaderLibrary;
    MTL::Function* m_fragmentFunction;

    MTL::RenderPipelineState* m_state;
    MTL::ComputePipelineState* m_computeState;
    MTL::DepthStencilState* m_stencilState;

    MTL::Buffer* m_vertexBuffer;
    MTL::Buffer* m_indexBuffer;

    MTL::Buffer* m_instanceDataBuffer[kMaxFramesInFlight];
    MTL::Buffer* m_cameraDataBuffer[kMaxFramesInFlight];

    MTL::Texture* m_texture;
    MTL::Buffer* m_textureAnimationBuffer;

    MTL::Texture* m_depthTexture = nullptr;

    std::vector<Model> m_importedModels;
    Model m_importedModel;

    MTL::Buffer* m_frameData[3];
    float m_angle;
    int m_frame;
    dispatch_semaphore_t m_semaphore;
    static const int kMaxFramesInFlight;
    uint m_animationIndex;

    std::vector<directionalLight> m_dirLights;
    MTL::Buffer* m_dirLightsBuffer;
    
    std::vector<pointLight> m_pointLights;
    MTL::Buffer* m_pointLightsBuffer;
    
    MTL::Texture* m_cubeMapTexture;
    MTL::Buffer* m_cubeMapBuffer;
    MTL::RenderPipelineState* m_cubemapState;
    
    Gizmo m_gizmo;
    MTL::RenderPipelineState* m_gizmoState;
    
    ImGuiLayer m_imguiLayer;
};
