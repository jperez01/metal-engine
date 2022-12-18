#include <simd/simd.h>
#include <Metal/Metal.hpp>

#include <string>
#include <vector>

struct Vertex {
    simd::float3 position;
    simd::float3 normal;
    simd::float2 texCoords;
};

struct Texture {
    MTL::Texture* actualTexture;
    std::string type;
    std::string path;
};

struct TypeEndpoints {
    int diffuse;
    int specular;
    int normal;
    int height;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    TypeEndpoints endpoints;
    
    Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<Texture>& textures, TypeEndpoints& endpoints);
    void setupMesh(MTL::Device* device, MTL::Function* function);
    void draw(MTL::RenderCommandEncoder* encoder);

private:
    
    MTL::Buffer* m_verticesBuffer;
    MTL::Buffer* m_indicesBuffer;
    MTL::Buffer* m_endpointBuffer;
    MTL::Buffer* m_argTextureBuffer;
};
