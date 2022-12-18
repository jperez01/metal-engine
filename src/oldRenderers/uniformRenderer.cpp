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
    buildFrameData();

    m_semaphore = dispatch_semaphore_create(Renderer::kMaxFramesInFlight);
}

Renderer::~Renderer() {
    m_shaderLibrary->release();
    m_argBuffer->release();
    m_vertexPositionBuffer->release();
    m_vertexColorsBuffer->release();

    for (int i = 0; i < Renderer::kMaxFramesInFlight; i++) {
        m_frameData[i]->release();
    }

    m_state->release();
    m_device->release();
    m_commandQueue->release();
}

void Renderer::buildShaders() {
    NS::StringEncoding encoding = NS::StringEncoding::UTF8StringEncoding;

    std::string shaderInfo = util::readFileIntoString("../shaders/uniformShader.metal");
    
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
    const size_t NumVertices = 3;

    simd::float3 positions[NumVertices] = {
        { -0.8f,  0.8f, 0.0f },
        {  0.0f, -0.8f, 0.0f },
        { +0.8f,  0.8f, 0.0f }
    };

    simd::float3 colors[NumVertices] = {
        {  1.0, 0.3f, 0.2f },
        {  0.8f, 1.0, 0.0f },
        {  0.8f, 0.0f, 1.0 }
    };

    const size_t positionDataSize = NumVertices * sizeof(simd::float3);
    const size_t colorDataSize = NumVertices * sizeof(simd::float3);

    MTL::Buffer* vertexPositionBuffer = m_device->newBuffer(positionDataSize, 
        MTL::ResourceStorageModeManaged);
    MTL::Buffer* vertexColorBuffer = m_device->newBuffer(colorDataSize,
        MTL::ResourceStorageModeManaged);

    m_vertexPositionBuffer = vertexPositionBuffer;
    m_vertexColorsBuffer = vertexColorBuffer;

    memcpy(m_vertexPositionBuffer->contents(), positions, positionDataSize);
    memcpy(m_vertexColorsBuffer->contents(), colors, colorDataSize);

    m_vertexPositionBuffer->didModifyRange(NS::Range::Make(0, m_vertexPositionBuffer->length()));
    m_vertexColorsBuffer->didModifyRange(NS::Range::Make(0, m_vertexColorsBuffer->length()));

    NS::StringEncoding encoding = NS::StringEncoding::UTF8StringEncoding;

    MTL::Function* vertexFn = m_shaderLibrary->newFunction(NS::String::string("vertexMain", encoding));
    MTL::ArgumentEncoder* encoder = vertexFn->newArgumentEncoder(0);

    MTL::Buffer* argBuffer = m_device->newBuffer(encoder->encodedLength(), MTL::ResourceStorageModeManaged);
    m_argBuffer = argBuffer;

    encoder->setArgumentBuffer(m_argBuffer, 0);

    encoder->setBuffer(m_vertexPositionBuffer, 0, 0);
    encoder->setBuffer(m_vertexColorsBuffer, 0, 1);

    m_argBuffer->didModifyRange(NS::Range::Make(0, argBuffer->length()));

    vertexFn->release();
    encoder->release();
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
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame+1) % Renderer::kMaxFramesInFlight;
    MTL::Buffer* frameDataBuffer = m_frameData[m_frame];

    MTL::CommandBuffer* cmd = m_commandQueue->commandBuffer();

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    Renderer* renderer = this;
    cmd->addCompletedHandler([renderer](MTL::CommandBuffer* cmd) {
        dispatch_semaphore_signal(renderer->m_semaphore);
    });

    m_angle += 0.01f;
    reinterpret_cast<FrameData*>(frameDataBuffer->contents())->angle = m_angle;
    frameDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(FrameData)));

    MTL::RenderPassDescriptor* descriptor = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* encoder = cmd->renderCommandEncoder(descriptor);

    encoder->setRenderPipelineState(m_state);
    encoder->setVertexBuffer(m_argBuffer, 0, 0);
    encoder->useResource(m_vertexPositionBuffer, MTL::ResourceUsageRead);
    encoder->useResource(m_vertexColorsBuffer, MTL::ResourceUsageRead);

    encoder->setVertexBuffer(frameDataBuffer, 0, 1);
    encoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 
        NS::UInteger(0), NS::UInteger(3));

    encoder->endEncoding();
    cmd->presentDrawable(view->currentDrawable());
    cmd->commit();

    pool->release();
}