#include <iostream>
#include <cassert>
#include <simd/simd.h>

#include "utility/math.h"
#include "utility/fileIO.h"
#include "utility/importUtils.hpp"

#include "imgui.h"
#include "imgui_impl_metal.h"
#include "imgui_tables.cpp"
#include "imgui_impl_sdl.h"

#include <ImGuiFileDialog/ImGuiFileDialog.h>

#include "renderer.h"
#include <SDL2/SDL.h>
#include <thread>
#include <future>

const int Renderer::kMaxFramesInFlight = 3;

namespace shader_types {
    struct VertexData {
        simd::float3 position;
        simd::float3 normal;
        simd::float2 texCoord;
    };

    struct InstanceData {
        simd::float4x4 instanceTransform;
        simd::float4 instanceColor;
        simd::float3x3 instanceNormalTransform;
    };

    struct CameraData {
        simd::float3 position;
        simd::float4x4 view;
        simd::float4x4 perspective;
        simd::float4x4 modifiedView;
    };

    struct DirectionalLight {
        simd::float3 direction;
        simd::float3 diffuse;
        simd::float3 specular;
        simd::float3 ambient;
    };

    struct PointLight {
        simd::float3 position;
        simd::float3 diffuse;
        simd::float3 specular;
        simd::float3 ambient;
    };

    struct LightInfo {
        simd::int1 numPoints;
        simd::int1 numDirections;
    };
}

Renderer::Renderer() : 
    m_angle(0.0f),
    m_frame(0),
    m_animationIndex(0) {
    initWindow();
    m_commandQueue = m_device->newCommandQueue();
    m_camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    
    buildShaders();
    buildComputePipeline();
    buildDepthStencilStates();
    buildTextures();
    buildBuffers();
    importModel();
        
    createLights();
    buildCubemap();
    m_semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);
}

Renderer::~Renderer() {
    m_textureAnimationBuffer->release();
    m_texture->release();
    m_shaderLibrary->release();
    m_stencilState->release();
    m_vertexBuffer->release();
    
    m_fragmentFunction->release();

    for (int i = 0; i < kMaxFramesInFlight; i++) {
        m_instanceDataBuffer[i]->release();
    }
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        m_cameraDataBuffer[i]->release();
    }

    m_indexBuffer->release();
    m_computeState->release();
    m_state->release();
    m_commandQueue->release();
    m_device->release();
    
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Renderer::initWindow() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s \n", SDL_GetError());
    }
    
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    
    SDL_Window* window = SDL_CreateWindow("Metal Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL) {
        printf("Error creating window; %s \n", SDL_GetError());
    }
    m_window = window;
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Error creating renderer: %s \n", SDL_GetError());
    }
    m_renderer = renderer;
    
    CA::MetalLayer* layer = (CA::MetalLayer*) SDL_RenderGetMetalLayer(renderer);
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm_sRGB);
    MTL::Device* device = layer->device()->retain();
    m_device = device;
    m_layer = layer->retain();
    
    ImGui_ImplMetal_Init(m_device);
    ImGui_ImplSDL2_InitForMetal(m_window);
}

void Renderer::run() {
    auto& io = ImGui::GetIO();
    
    SDL_Event event;
    bool done = false;
    
    while (!done) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            
            if (io.WantCaptureMouse) break;
            if (event.type == SDL_QUIT) {
                done = true;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode type = event.key.keysym.sym;
                
                if (type == SDLK_LEFT) m_camera.processKeyboard(LEFT, m_cameraInfo.deltaTime);
                if (type == SDLK_RIGHT) m_camera.processKeyboard(RIGHT, m_cameraInfo.deltaTime);
                if (type == SDLK_UP) m_camera.processKeyboard(FORWARD, m_cameraInfo.deltaTime);
                if (type == SDLK_DOWN) m_camera.processKeyboard(BACKWARD, m_cameraInfo.deltaTime);
            } else if (event.type == SDL_MOUSEMOTION) {
                float mouseX = event.motion.x;
                float mouseY = event.motion.y;
                
                if (m_cameraInfo.firstMouse) {
                    m_cameraInfo.lastX = mouseX;
                    m_cameraInfo.lastY = mouseY;
                    m_cameraInfo.firstMouse = false;
                }
                
                float xoffset = mouseX - m_cameraInfo.lastX;
                float yoffset = m_cameraInfo.lastY - mouseY;
                
                m_cameraInfo.lastX = mouseX;
                m_cameraInfo.lastY = mouseY;
                m_camera.processMouseMovement(xoffset, yoffset);
            } else if (event.type == SDL_MOUSEWHEEL) {
                float scrollY = event.wheel.y;
                m_camera.processMouseScroll(scrollY);
            }
            
        }
        
        float currentFrame = static_cast<float>(SDL_GetTicks());
        m_cameraInfo.deltaTime = currentFrame - m_cameraInfo.lastFrame;
        m_cameraInfo.deltaTime *= 0.1;
        m_cameraInfo.lastFrame = currentFrame;
        
        int width = 0, height = 0;
        SDL_GetRendererOutputSize(m_renderer, &width, &height);
        
        m_layer->setDrawableSize(CGSizeMake(width, height));
        
        auto layer = m_layer->nextDrawable();
        drawModelOnly(layer);
        layer->release();
    }
}

void Renderer::createLights() {
    pointLight point = {};
    m_pointLights.push_back(point);
    
    directionalLight direction = {};
    m_dirLights.push_back(direction);
}

void Renderer::buildShaders() {
    NS::StringEncoding encoding = NS::StringEncoding::UTF8StringEncoding;

    std::string shaderInfo = util::readFileIntoString("/Users/juanperez/Documents/projects/metal-engine/shaders/modelShader.metal");
    
    NS::Error* error = nullptr;
    MTL::Library* library = m_device->newLibrary(
        NS::String::string(shaderInfo.c_str(), encoding), nullptr, &error
    );

    if (!library) {
        const char* errorString = error->localizedDescription()->utf8String();
        printf("%s \n", errorString);
        assert(false);
    }

    MTL::Function* vertexFn = library->newFunction(NS::String::string("vertexMain", encoding));
    MTL::Function* fragmentFn = library->newFunction(NS::String::string("fragmentMain", encoding));

    MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    descriptor->setVertexFunction(vertexFn);
    descriptor->setFragmentFunction(fragmentFn);
    descriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);

    m_state = m_device->newRenderPipelineState(descriptor, &error);
    if (!m_state) {
        printf("%s", error->localizedDescription()->utf8String());
        assert(false);
    }

    m_shaderLibrary = library;
    m_fragmentFunction = fragmentFn;

    vertexFn->release();
    descriptor->release();
}

void Renderer::buildComputePipeline() {
    std::string kernelInfo = util::readFileIntoString("/Users/juanperez/Documents/projects/metal-engine/shaders/computeShader.metal");
    
    NS::Error* error = nullptr;
    
    MTL::Library* library = m_device->newLibrary(
         NS::String::string(kernelInfo.c_str(), NS::UTF8StringEncoding), nullptr, &error);
    if (!library) {
        printf("%s \n", error->localizedDescription()->utf8String());
        assert(false);
    }
    
    MTL::Function* mandelBrotFn = library->newFunction(NS::String::string("mandelbrot_set", NS::UTF8StringEncoding));
    m_computeState = m_device->newComputePipelineState(mandelBrotFn, &error);
    if (!m_computeState) {
        printf("%s \n", error->localizedDescription()->utf8String());
        assert(false);
    }
    
    mandelBrotFn->release();
    library->release();
}

void Renderer::buildDepthStencilStates() {
    MTL::DepthStencilDescriptor* descriptor = MTL::DepthStencilDescriptor::alloc()->init();
    descriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLessEqual);
    descriptor->setDepthWriteEnabled(true);

    m_stencilState = m_device->newDepthStencilState(descriptor);

    descriptor->release();
}

void Renderer::buildTextures() {
    MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
    descriptor->setWidth(kTextureWidth);
    descriptor->setHeight(kTextureHeight);
    descriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    descriptor->setTextureType(MTL::TextureType2D);
    descriptor->setStorageMode(MTL::StorageModeManaged);
    descriptor->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    
    MTL::Texture* texture = m_device->newTexture(descriptor);
    m_texture = texture;
    
    descriptor->release();
}

void Renderer::buildBuffers() {
    const float s = 0.5f;

    shader_types::VertexData verts[] = {
        //                                         Texture
        //   Positions           Normals         Coordinates
        { { -s, -s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
        { { +s, -s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
        { { +s, +s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
        { { -s, +s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },

        { { +s, -s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { +s, -s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { +s, +s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { +s, +s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },

        { { +s, -s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
        { { -s, -s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
        { { -s, +s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
        { { +s, +s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },

        { { -s, -s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { -s, -s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { -s, +s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { -s, +s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },

        { { -s, +s, +s }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },
        { { +s, +s, +s }, {  0.f,  1.f,  0.f }, { 1.f, 1.f } },
        { { +s, +s, -s }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
        { { -s, +s, -s }, {  0.f,  1.f,  0.f }, { 0.f, 0.f } },

        { { -s, -s, -s }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } },
        { { +s, -s, -s }, {  0.f, -1.f,  0.f }, { 1.f, 1.f } },
        { { +s, -s, +s }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
        { { -s, -s, +s }, {  0.f, -1.f,  0.f }, { 0.f, 0.f } }
    };

    uint16_t indices[] = {
         0,  1,  2,  2,  3,  0, /* front */
         4,  5,  6,  6,  7,  4, /* right */
         8,  9, 10, 10, 11,  8, /* back */
        12, 13, 14, 14, 15, 12, /* left */
        16, 17, 18, 18, 19, 16, /* top */
        20, 21, 22, 22, 23, 20, /* bottom */
    };

    const size_t vertexDataSize = sizeof(verts);
    const size_t indexDataSize = sizeof(indices);

    MTL::Buffer* vertexBuffer = m_device->newBuffer(vertexDataSize, 
        MTL::ResourceStorageModeManaged);
    MTL::Buffer* indexBuffer = m_device->newBuffer(indexDataSize,
        MTL::ResourceStorageModeManaged);

    m_vertexBuffer = vertexBuffer;
    m_indexBuffer = indexBuffer;

    memcpy(m_vertexBuffer->contents(), verts, vertexDataSize);
    memcpy(m_indexBuffer->contents(), indices, indexDataSize);

    m_vertexBuffer->didModifyRange(NS::Range::Make(0, m_vertexBuffer->length()));
    m_indexBuffer->didModifyRange(NS::Range::Make(0, m_indexBuffer->length()));

    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof(shader_types::InstanceData);
    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        m_instanceDataBuffer[i] = m_device->newBuffer(instanceDataSize, MTL::ResourceStorageModeManaged);
    }

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof(shader_types::CameraData);
    for (size_t i = 0; i < kMaxFramesInFlight; i++) {
        m_cameraDataBuffer[i] = m_device->newBuffer(cameraDataSize, MTL::ResourceStorageModeManaged);
    }
    
    const size_t dirLightSize = 10 * sizeof(shader_types::DirectionalLight);
    m_dirLightsBuffer = m_device->newBuffer(dirLightSize, MTL::ResourceStorageModeManaged);
    
    const size_t pointLightSize = 10 * sizeof(shader_types::PointLight);
    m_pointLightsBuffer = m_device->newBuffer(pointLightSize, MTL::ResourceStorageModeManaged);
    
    m_textureAnimationBuffer = m_device->newBuffer(sizeof(uint), MTL::ResourceStorageModeManaged);
}

struct FrameData {
    float angle;
};

void Renderer::buildFrameData() {
    for (int i = 0; i < Renderer::kMaxFramesInFlight; i++) {
        m_frameData[i] = m_device->newBuffer(sizeof(FrameData), MTL::ResourceStorageModeManaged);
    }
}

void Renderer::buildCubemap() {
    m_cubeMapTexture = importUtils::cubemapFromFile("/Users/juanperez/Documents/projects/metal-engine/resources/skybox", m_device);
    
    simd::float3 skyboxVertices[] = {
        // positions
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},

        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},

        {-1.0f,  1.0f, -1.0f},
        {1.0f,  1.0f, -1.0f},
        {1.0f,  1.0f,  1.0f},
        {1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f},

        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {1.0f, -1.0f, -1.0f},
        {1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {1.0f, -1.0f,  1.0f}
    };
    
    const size_t skyboxVertSize = sizeof(skyboxVertices);
    m_cubeMapBuffer = m_device->newBuffer(skyboxVertSize, MTL::ResourceStorageModeManaged);
    
    memcpy(m_cubeMapBuffer->contents(), skyboxVertices, skyboxVertSize);
    m_cubeMapBuffer->didModifyRange(NS::Range::Make(0, m_cubeMapBuffer->length()));
    
    NS::StringEncoding encoding = NS::StringEncoding::UTF8StringEncoding;

    std::string shaderInfo = util::readFileIntoString("/Users/juanperez/Documents/projects/metal-engine/shaders/cubemapShader.metal");
    
    NS::Error* error = nullptr;
    MTL::Library* library = m_device->newLibrary(
        NS::String::string(shaderInfo.c_str(), encoding), nullptr, &error
    );

    if (!library) {
        const char* errorString = error->localizedDescription()->utf8String();
        printf("%s \n", errorString);
        assert(false);
    }

    MTL::Function* vertexFn = library->newFunction(NS::String::string("vertexMain", encoding));
    MTL::Function* fragmentFn = library->newFunction(NS::String::string("fragmentMain", encoding));

    MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    descriptor->setVertexFunction(vertexFn);
    descriptor->setFragmentFunction(fragmentFn);
    descriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);

    m_cubemapState = m_device->newRenderPipelineState(descriptor, &error);
    if (!m_cubemapState) {
        printf("%s", error->localizedDescription()->utf8String());
        assert(false);
    }

    vertexFn->release();
    fragmentFn->release();
    descriptor->release();
}

void Renderer::generateMandelbrotTexture(MTL::CommandBuffer* cmd) {
    uint* ptr = reinterpret_cast<uint*>(m_textureAnimationBuffer->contents());
    *ptr = (m_animationIndex++) % 5000;
    m_textureAnimationBuffer->didModifyRange(NS::Range::Make(0, sizeof(uint)));
    
    MTL::ComputeCommandEncoder* computeEncoder = cmd->computeCommandEncoder();
    
    computeEncoder->setComputePipelineState(m_computeState);
    computeEncoder->setTexture(m_texture, 0);
    computeEncoder->setBuffer(m_textureAnimationBuffer, 0, 0);
    
    MTL::Size gridSize(kTextureWidth, kTextureHeight, 1);
    
    NS::UInteger threadGroupSize = m_computeState->maxTotalThreadsPerThreadgroup();
    MTL::Size numThreads(threadGroupSize, 1, 1);
    
    computeEncoder->dispatchThreads(gridSize, numThreads);
    computeEncoder->endEncoding();

}

void Renderer::importModel() {

}

void Renderer::draw(CA::MetalDrawable* drawable) {
    using simd::float4x4;
    using simd::float4;
    using simd::float3;

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame+1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* instanceDataBuffer = m_instanceDataBuffer[m_frame];

    MTL::CommandBuffer* cmd = m_commandQueue->commandBuffer();

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    Renderer* renderer = this;
    cmd->addCompletedHandler([renderer](MTL::CommandBuffer* cmd) {
        dispatch_semaphore_signal(renderer->m_semaphore);
    });

    m_angle += 0.002f;

    const float scl = 0.5f;
    shader_types::InstanceData* instanceData = 
        reinterpret_cast<shader_types::InstanceData*>(instanceDataBuffer->contents());

    float3 objectPosition = { 0.0f, 0.0f, -5.0f };

    float4x4 rt = math::makeTranslate(objectPosition);
    float4x4 rr1 = math::makeYRotate(-m_angle);
    float4x4 rr0 = math::makeXRotate(m_angle * 0.5);

    float3 inverseVector = { -objectPosition.x, -objectPosition.y, -objectPosition.z};
    float4x4 rtInverse = math::makeTranslate(inverseVector);
    float4x4 fullObjectRotate = rt * rr1 * rr0 * rtInverse;

    size_t ix = 0, iy = 0, iz = 0;
    for (size_t i = 0; i < kNumInstances; i++) {
        if (ix == kInstanceRows) {
            ix = 0;
            iy += 1;
        }
        if (iy == kInstanceRows) {
            iy = 0;
            iz += 1;
        }

        float4x4 scale = math::makeScale((float3){scl, scl, scl});
        float4x4 zrot = math::makeZRotate(m_angle * sinf((float)ix));
        float4x4 yrot = math::makeYRotate(m_angle * cosf((float)iy));

        float x = ((float)ix - (float)kInstanceRows/2.0f) * (2.0f * scl) + scl;
        float y = ((float)iy - (float)kInstanceRows/2.0f) * (2.0f * scl) + scl;
        float z = ((float)iz - (float)kInstanceRows/2.0f) * (2.0f * scl) + scl;
        float3 offsets = (float3){x, y, z};
        float4x4 translate = math::makeTranslate(math::add(objectPosition, offsets));

        instanceData[i].instanceTransform = fullObjectRotate * translate * yrot * zrot * scale;
        instanceData[i].instanceNormalTransform = math::discardTranslation(instanceData[i].instanceTransform);

        float divNumInstances = i / (float)kNumInstances;
        float r = divNumInstances,
            g = 1.0f - r,
            b = sinf(M_PI * 2.0f * divNumInstances);
        instanceData[i].instanceColor = (float4){r, g, b, 1.0f};

        ix += 1;
    }
    instanceDataBuffer->didModifyRange(NS::Range::Make(0, instanceDataBuffer->length()));

    // Setup Camera Data
    MTL::Buffer* cameraDataBuffer = m_cameraDataBuffer[m_frame];
    shader_types::CameraData* cameraData = reinterpret_cast<shader_types::CameraData*>(cameraDataBuffer->contents());
    cameraData->view = m_camera.getViewMatrix();
    cameraData->perspective = m_camera.getPerspectiveMatrix(1280.0 / 720.0);
    cameraData->position = m_camera.getPosition();
    cameraDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::CameraData)));
    
    generateMandelbrotTexture(cmd);

    // Start actual rendering
    MTL::RenderPassDescriptor* descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
    
    auto color_attachment = descriptor->colorAttachments()->object(0);
    color_attachment->setClearColor(MTL::ClearColor(1, 1, 1, 0));
    color_attachment->setTexture(drawable->texture());
    color_attachment->setLoadAction(MTL::LoadAction::LoadActionClear);
    color_attachment->setStoreAction(MTL::StoreAction::StoreActionStore);
    
    if (m_depthTexture == nullptr) {
        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setWidth(2560);
        textureDescriptor->setHeight(1440);
        textureDescriptor->setPixelFormat(MTL::PixelFormatDepth16Unorm);
        textureDescriptor->setTextureType(MTL::TextureType2D);
        textureDescriptor->setStorageMode(MTL::StorageModeManaged);
        textureDescriptor->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
        
        MTL::Texture* texture = m_device->newTexture(textureDescriptor);
        m_depthTexture = texture->retain();
    }
    
    descriptor->depthAttachment()->setTexture(m_depthTexture);

    
    MTL::RenderCommandEncoder* encoder = cmd->renderCommandEncoder(descriptor);
    
    encoder->setViewport(MTL::Viewport {
        0.0f, 0.0f,
        2560, 1440,
        0.0f, 1.0f
    });
    
    encoder->setDepthStencilState(m_stencilState);
    encoder->setRenderPipelineState(m_state);

    encoder->setVertexBuffer(m_vertexBuffer, 0, 0);
    encoder->setVertexBuffer(instanceDataBuffer, 0, 1);
    encoder->setVertexBuffer(cameraDataBuffer, 0, 2);
    
    encoder->setFragmentTexture(m_texture, 0);

    encoder->setCullMode(MTL::CullModeBack);
    encoder->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);

    encoder->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
        6 * 6, MTL::IndexType::IndexTypeUInt16,
        m_indexBuffer,
        0, kNumInstances);
    
    ImGui_ImplMetal_NewFrame(descriptor);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    ImGui::ShowDemoWindow();
    
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd, encoder);

    encoder->endEncoding();
    cmd->presentDrawable(drawable);
    cmd->commit();

    pool->release();
}

void Renderer::drawModelOnly(CA::MetalDrawable* drawable) {
    using simd::float4x4;
    using simd::float4;
    using simd::float3;

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame+1) % Renderer::kMaxFramesInFlight;

    MTL::CommandBuffer* cmd = m_commandQueue->commandBuffer();

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    Renderer* renderer = this;
    cmd->addCompletedHandler([renderer](MTL::CommandBuffer* cmd) {
        dispatch_semaphore_signal(renderer->m_semaphore);
    });

    // Setup Camera Data
    MTL::Buffer* cameraDataBuffer = m_cameraDataBuffer[m_frame];
    shader_types::CameraData* cameraData = reinterpret_cast<shader_types::CameraData*>(cameraDataBuffer->contents());
    cameraData->view = m_camera.getViewMatrix();
    cameraData->perspective = m_camera.getPerspectiveMatrix(1280.0 / 720.0);
    cameraData->position = m_camera.getPosition();
    cameraData->modifiedView = m_camera.getModifiedView();
    cameraDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::CameraData)));
    
    size_t numDirLights = m_dirLights.size();
    for (size_t i = 0; i < numDirLights; i++) {
        directionalLight &dirLightInfo = m_dirLights[i];
        shader_types::DirectionalLight* dirLight = reinterpret_cast<shader_types::DirectionalLight*>(m_dirLightsBuffer->contents());
        dirLight += i;
        dirLight->direction = simd_make_float3(dirLightInfo.direction[0], dirLightInfo.direction[1], dirLightInfo.direction[2]);
        dirLight->ambient = simd_make_float3(dirLightInfo.ambient[0], dirLightInfo.ambient[1], dirLightInfo.ambient[2]);
        dirLight->specular = simd_make_float3(dirLightInfo.specular[0], dirLightInfo.specular[1], dirLightInfo.specular[2]);
        dirLight->diffuse = simd_make_float3(dirLightInfo.diffuse[0], dirLightInfo.diffuse[1], dirLightInfo.diffuse[2]);
    }
    if (numDirLights > 0) m_dirLightsBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::DirectionalLight) * numDirLights));
    
    size_t numPointLights = m_pointLights.size();
    for (size_t i = 0; i < numPointLights; i++) {
        pointLight &pointLightInfo = m_pointLights[i];
        shader_types::PointLight* pointLight = reinterpret_cast<shader_types::PointLight*>(m_pointLightsBuffer->contents());
        pointLight += i;
        pointLight->position = simd_make_float3(pointLightInfo.position[0], pointLightInfo.position[1], pointLightInfo.position[2]);
        pointLight->ambient = simd_make_float3(pointLightInfo.ambient[0], pointLightInfo.ambient[1], pointLightInfo.ambient[2]);
        pointLight->specular = simd_make_float3(pointLightInfo.specular[0], pointLightInfo.specular[1], pointLightInfo.specular[2]);
        pointLight->diffuse = simd_make_float3(pointLightInfo.diffuse[0], pointLightInfo.diffuse[1], pointLightInfo.diffuse[2]);
    }
    if (numPointLights > 0) m_pointLightsBuffer->didModifyRange(NS::Range::Make(0, sizeof(shader_types::PointLight) * numPointLights));
    
    shader_types::LightInfo lightInfo = {};
    lightInfo.numDirections = numDirLights;
    lightInfo.numPoints = numPointLights;

    // Start actual rendering
    MTL::RenderPassDescriptor* descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
    
    auto color_attachment = descriptor->colorAttachments()->object(0);
    color_attachment->setClearColor(MTL::ClearColor(1, 0.5, 1, 0));
    color_attachment->setTexture(drawable->texture());
    color_attachment->setLoadAction(MTL::LoadAction::LoadActionClear);
    color_attachment->setStoreAction(MTL::StoreAction::StoreActionStore);
    
    if (m_depthTexture == nullptr) {
        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
        textureDescriptor->setWidth(2560);
        textureDescriptor->setHeight(1440);
        textureDescriptor->setPixelFormat(MTL::PixelFormatDepth16Unorm);
        textureDescriptor->setTextureType(MTL::TextureType2D);
        textureDescriptor->setStorageMode(MTL::StorageModeManaged);
        textureDescriptor->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
        
        MTL::Texture* texture = m_device->newTexture(textureDescriptor);
        m_depthTexture = texture->retain();
        
        textureDescriptor->release();
    }
    
    descriptor->depthAttachment()->setTexture(m_depthTexture);
    
    MTL::RenderCommandEncoder* encoder = cmd->renderCommandEncoder(descriptor);
    
    encoder->setViewport(MTL::Viewport {
        0.0f, 0.0f,
        2560, 1440,
        0.0f, 1.0f
    });
    
    // Rendering model
    encoder->setDepthStencilState(m_stencilState);
    encoder->setRenderPipelineState(m_state);
    
    encoder->setVertexBuffer(cameraDataBuffer, 0, 2);
    
    encoder->setFragmentBuffer(m_pointLightsBuffer, 0, 2);
    encoder->setFragmentBuffer(m_dirLightsBuffer, 0, 3);
    encoder->setFragmentBytes(&lightInfo, sizeof(shader_types::LightInfo), 4);

    encoder->setCullMode(MTL::CullModeNone);
    encoder->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);

    for (Model& model : m_importedModels) {
        model.draw(encoder);
    }
    
    // Rendering cubemap
    encoder->setRenderPipelineState(m_cubemapState);
    
    encoder->setVertexBuffer(m_cubeMapBuffer, 0, 0);
    encoder->setVertexBuffer(cameraDataBuffer, 0, 1);
    encoder->setFragmentTexture(m_cubeMapTexture, 0);
    
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0), 36);
    
    // Rendering ImGui
    ImGui_ImplMetal_NewFrame(descriptor);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("Info");
    size_t numLights = m_dirLights.size();
    
    if (ImGui::CollapsingHeader("Directional Lights")) {
        for (size_t i = 0; i < numLights; i++) {
            directionalLight &currentLight = m_dirLights[i];
            
            std::string name = "D Light " + std::to_string(i);
            if (ImGui::TreeNode(name.c_str())) {
                ImGui::SliderFloat3("Direction", &currentLight.direction[0], -1.0f, 1.0f);
                ImGui::SliderFloat3("Ambient", &currentLight.ambient[0], 0.0, 1.0);
                ImGui::SliderFloat3("Diffuse", &currentLight.diffuse[0], 0.0, 1.0);
                ImGui::SliderFloat3("Specular", &currentLight.specular[0], 0.0, 1.0);
                ImGui::TreePop();
            }
        }
    }
    
    numLights = m_pointLights.size();
    if (ImGui::CollapsingHeader("Point Lights")) {
        for (size_t i = 0; i < numLights; i++) {
            auto &currentPoint = m_pointLights[i];
            
            std::string name = "P Light " + std::to_string(i);
            if (ImGui::TreeNode(name.c_str())) {
                ImGui::SliderFloat3("Position", &currentPoint.position[0], -10.0f, 10.0f);
                ImGui::SliderFloat3("Ambient", &currentPoint.ambient[0], 0.0, 1.0);
                ImGui::SliderFloat3("Diffuse", &currentPoint.diffuse[0], 0.0, 1.0);
                ImGui::SliderFloat3("Specular", &currentPoint.specular[0], 0.0, 1.0);
                ImGui::TreePop();
            }
        }
    }
    if (ImGui::Button("Add Point Light")) {
        struct pointLight newPoint = {};
        m_pointLights.push_back(newPoint);
    }
    
    if (ImGui::Button("Add Directional Light")) {
        directionalLight newLight = {};
        m_dirLights.push_back(newLight);
    }
    
    if (ImGui::Button("Open File Dialog")) ImGuiFileDialog::Instance()->OpenDialog("ChooseFileKey", "Choose File", ".obj", ".");
    
    if (ImGuiFileDialog::Instance()->Display("ChooseFileKey")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            
            auto result = std::async(std::launch::async, &Renderer::asyncImportModel, this, filePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }
    
    ImGui::End();
    
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmd, encoder);
    
    encoder->endEncoding();
    cmd->presentDrawable(drawable);
    cmd->commit();

    pool->release();
}

void Renderer::asyncImportModel(std::string path) {
    Model importedModel(path, m_device);
    importedModel.setupMeshBuffers(m_device, m_fragmentFunction);
    
    m_importedModels.push_back(importedModel);
}

void asyncImportModel(std::string& path, std::vector<Model>& importedModels, MTL::Device* device, MTL::Function* fragmentFn) {
    Model importedModel(path, device);
    importedModel.setupMeshBuffers(device, fragmentFn);
    
    importedModels.push_back(importedModel);
}
