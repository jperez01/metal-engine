#include "mesh.h"

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures, TypeEndpoints& endpoints) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    this->endpoints = endpoints;
}

void Mesh::setupMesh(MTL::Device* device, MTL::Function* function) {
    const size_t vertexDataSize = vertices.size() * sizeof(Vertex);
    const size_t indexDataSize = indices.size() * sizeof(unsigned int);
    const size_t endpointsDataSize = sizeof(int) * 4;
    
    MTL::Buffer* vertexBuffer = device->newBuffer(vertexDataSize, MTL::ResourceStorageModeManaged);
    MTL::Buffer* indexBuffer = device->newBuffer(indexDataSize, MTL::ResourceStorageModeManaged);
    
    MTL::Buffer* endpointBuffer = device->newBuffer(endpointsDataSize, MTL::ResourceStorageModeManaged);
    
    m_verticesBuffer = vertexBuffer;
    m_indicesBuffer = indexBuffer;
    m_endpointBuffer = endpointBuffer;
    
    memcpy(m_verticesBuffer->contents(), vertices.data(), vertexDataSize);
    memcpy(m_indicesBuffer->contents(), indices.data(), indexDataSize);
    memcpy(m_endpointBuffer->contents(), &endpoints, endpointsDataSize);
    
    m_verticesBuffer->didModifyRange(NS::Range::Make(0, m_verticesBuffer->length()));
    m_indicesBuffer->didModifyRange(NS::Range::Make(0, m_indicesBuffer->length()));
    m_endpointBuffer->didModifyRange(NS::Range::Make(0, m_endpointBuffer->length()));
    
    MTL::ArgumentEncoder* argEncoder = function->newArgumentEncoder(1);
    int totalSize = argEncoder->encodedLength() * textures.size();
    
    MTL::Buffer* argBuffer = device->newBuffer(totalSize, MTL::ResourceStorageModeManaged);
    
    for (unsigned int i = 0; i < textures.size(); i++) {
        Texture currentTexture = textures[i];
        
        argEncoder->setArgumentBuffer(argBuffer, argEncoder->encodedLength() * i);
        argEncoder->setTexture(currentTexture.actualTexture, 0);
    }
    
    argBuffer->didModifyRange(NS::Range::Make(0, argBuffer->length()));
    
    m_argTextureBuffer = argBuffer;
}

void Mesh::draw(MTL::RenderCommandEncoder* encoder) {
    encoder->setVertexBuffer(m_verticesBuffer, 0, 0);
    encoder->setFragmentBuffer(m_endpointBuffer, 0, 0);
    encoder->setFragmentBuffer(m_argTextureBuffer, 0, 1);
    
    encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, indices.size(), MTL::IndexTypeUInt32, m_indicesBuffer, 0);
}
