#include <metal_stdlib>
using namespace metal;

struct v2f {
    float4 position [[position]];
    float3 texCoords;
};

struct VertexData {
    float3 position;
};

struct CameraData {
    float3 position;
    float4x4 view;
    float4x4 perspective;
    float4x4 modifiedView;
};

v2f vertex vertexMain(device const VertexData* vertexData [[buffer(0)]],
                      device const CameraData& cameraData [[buffer(1)]],
                      uint vertexId [[vertex_id]]) {
    v2f output;
    
    const device VertexData& currentVertex = vertexData[vertexId];
    float4 position = cameraData.perspective * cameraData.modifiedView * float4(currentVertex.position, 1.0);
    
    output.texCoords = currentVertex.position;
    output.position = position.xyww;
    
    return output;
}

float4 fragment fragmentMain(v2f in [[stage_in]],
                             texturecube<float, access::sample> cubemap [[texture(0)]]) {
    constexpr sampler s(address::repeat, filter::linear);
    
    float3 texCoords = float3(in.texCoords.x, in.texCoords.y, -in.texCoords.z);
    return float4(cubemap.sample(s, texCoords).rgb, 1.0);
}
