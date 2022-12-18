#include "renderer.h"
#include <iostream>
#include <cassert>
#include <simd/simd.h>
#include "utility/fileIO.h"

const int Renderer::kMaxFramesInFlight = 3;

Renderer::Renderer(MTL::Device* device) : 
    m_device(device->retain()),
    m_angle(0.0f),
    m_frame(0) {
    m_commandQueue = m_device->newCommandQueue();
    buildShaders();
    buildBuffers();

    m_semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);
}

Renderer::~Renderer() {
    m_shaderLibrary->release();
    m_vertexBuffer->release();

    for (int i = 0; i < kMaxFramesInFlight; i++) {
        m_instanceDataBuffer[i]->release();
    }

    m_indexBuffer->release();
    m_state->release();
    m_commandQueue->release();
    m_device->release();
}

namespace shader_types {
    struct InstanceData {
        simd::float4x4 instanceTransform;
        simd::float4 instanceColor;
    };
}

void Renderer::buildShaders() {
    NS::StringEncoding encoding = NS::StringEncoding::UTF8StringEncoding;

    std::string shaderInfo = util::readFileIntoString("../shaders/instanceShader.metal");
    
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

    m_state = m_device->newRenderPipelineState(descriptor, &error);
    if (!m_state) {
        printf("%s", error->localizedDescription()->utf8String());
        assert(false);
    }

    m_shaderLibrary = library;

    vertexFn->release();
    fragmentFn->release();
    descriptor->release();
}

void Renderer::buildBuffers() {
    const float s = 0.5f;

    simd::float3 verts[] = {
        { -s, -s, +s },
        { +s, -s, +s },
        { +s, +s, +s },
        { -s, +s, +s }
    };

    uint16_t indices[] = {
        0, 1, 2,
        2, 3, 0
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
}

struct FrameData {
    float angle;
};

void Renderer::buildFrameData() {
    for (int i = 0; i < Renderer::kMaxFramesInFlight; i++) {
        m_frameData[i] = m_device->newBuffer(sizeof(FrameData), MTL::ResourceStorageModeManaged);
    }
}

void Renderer::draw(MTK::View* view) {
    using simd::float4x4;
    using simd::float4;

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame+1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* instanceDataBuffer = m_instanceDataBuffer[m_frame];

    MTL::CommandBuffer* cmd = m_commandQueue->commandBuffer();

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    Renderer* renderer = this;
    cmd->addCompletedHandler([renderer](MTL::CommandBuffer* cmd) {
        dispatch_semaphore_signal(renderer->m_semaphore);
    });

    m_angle += 0.01f;

    const float scl = 0.3f;
    shader_types::InstanceData* instanceData = 
        reinterpret_cast<shader_types::InstanceData*>(instanceDataBuffer->contents());
    for (size_t i = 0; i < kNumInstances; i++) {
        float divNumInstances = i / (float)kNumInstances;
        float xoffset = (divNumInstances * 2.0f - 1.0f) + (1.0f / kNumInstances);
        float yoffset = sin(divNumInstances + m_angle) * 2.0f * M_PI;

        instanceData[i].instanceTransform = (float4x4){ 
            (float4){ scl * sinf(m_angle), scl * cosf(m_angle), 0.f, 0.f },
            (float4){ scl * cosf(m_angle), scl * -sinf(m_angle), 0.f, 0.f },
            (float4){ 0.f, 0.f, scl, 0.f },
            (float4){ xoffset, yoffset, 0.f, 1.f } 
        };

        float r = divNumInstances,
            g = 1.0f - r,
            b = sinf(M_PI * 2.0f * divNumInstances);
        instanceData[i].instanceColor = (float4){r, g, b, 1.0f};
    }
    instanceDataBuffer->didModifyRange(NS::Range::Make(0, instanceDataBuffer->length()));

    MTL::RenderPassDescriptor* descriptor = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* encoder = cmd->renderCommandEncoder(descriptor);

    encoder->setRenderPipelineState(m_state);
    encoder->setVertexBuffer(m_vertexBuffer, 0, 0);
    encoder->setVertexBuffer(instanceDataBuffer, 0, 1);

    encoder->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
        6, MTL::IndexType::IndexTypeUInt16,
        m_indexBuffer,
        0, kNumInstances);

    encoder->endEncoding();
    cmd->presentDrawable(view->currentDrawable());
    cmd->commit();

    pool->release();
}