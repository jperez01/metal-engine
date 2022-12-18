#include <metal_stdlib>
using namespace metal;

struct v2f {
    float4 position [[position]];
    float3 normal;
    float2 texcoord;
    float3 viewPos;
};

struct VertexData {
    float3 position;
    float3 normal;
    float2 texCoord;
};

struct DirectionalLight {
    float3 direction;
    float3 diffuse;
    float3 specular;
    float3 ambient;
};

struct PointLight {
    float3 position;
    float3 diffuse;
    float3 specular;
    float3 ambient;
};

struct CameraData {
    float3 position;
    float4x4 view;
    float4x4 perspective;
    float4x4 modifiedView;
};

struct SingleTexture {
    texture2d<float> texture;
};

struct TextureEndpoints {
    int diffuse;
    int specular;
    int normal;
    int height;
};

struct LightInfo {
    int numPoints;
    int numDirections;
};

v2f vertex vertexMain(device const VertexData* vertexData [[buffer(0)]],
                      device const CameraData& cameraData [[buffer(2)]], uint vertexId [[vertex_id]]) {
    v2f o;
    
    const device VertexData& vs = vertexData[vertexId];
    float4 pos = float4(vs.position, 1.0);
    o.position = cameraData.perspective * cameraData.view * pos;
    
    o.normal = vs.normal;
    o.texcoord = vs.texCoord.xy;
    o.viewPos = cameraData.position;
    
    return o;
}

float4 fragment fragmentMain(v2f in [[stage_in]], device TextureEndpoints& endpoints [[buffer(0)]], device SingleTexture* textures[[buffer(1)]],
                             device PointLight* pointLights [[buffer(2)]],
                             device DirectionalLight* directionLights [[buffer(3)]],
                             device const LightInfo& lightInfo [[buffer(4)]]) {
    constexpr sampler s(address::repeat, filter::linear);
    
    float3 total = float3(0.0);
    float3 viewDir = in.viewPos - in.position.xyz;
    viewDir = normalize(viewDir);
    
    for (int i = 0; i < lightInfo.numDirections; i++) {
        DirectionalLight light = directionLights[i];
        
        float diffuse = max(dot(in.normal, light.direction), 0.0);
        
        float3 reflectDir = reflect(-light.direction, in.normal);
        float specular = pow(max(dot(reflectDir, viewDir), 0.0), 10);
        
        for (int i = 0; i <= endpoints.diffuse; i++) {
            texture2d<float> currentTexture = textures[i].texture;
            float3 texel = currentTexture.sample(s, in.texcoord).rgb;
            
            total += light.diffuse * diffuse * texel;
        }
        
        for (int i = endpoints.diffuse+1; i <= endpoints.specular; i++) {
            texture2d<float> currentTexture = textures[i].texture;
            float3 texel = currentTexture.sample(s, in.texcoord).rgb;

            total += light.specular * specular * texel;
        }
        
        total += light.ambient;
    }
    
    for (int i = 0; i < lightInfo.numPoints; i++) {
        PointLight light = pointLights[i];
        float3 lightDir = normalize(light.position - in.position.xyz);
        
        float diffuse = max(dot(in.normal, lightDir), 0.0);
        
        float3 reflectDir = reflect(-lightDir, in.normal);
        float specular = pow(max(dot(reflectDir, viewDir), 0.0), 10);
        
        for (int j = 0; j <= endpoints.diffuse; j++) {
            texture2d<float> currentTexture = textures[j].texture;
            float3 texel = currentTexture.sample(s, in.texcoord).rgb;
            
            total += light.diffuse * diffuse * texel;
        }
        
        for (int j = endpoints.diffuse+1; j <= endpoints.specular; j++) {
            texture2d<float> currentTexture = textures[j].texture;
            float3 texel = currentTexture.sample(s, in.texcoord).rgb;

            total += light.specular * specular * texel;
        }
        
        total += light.ambient;
    }
    
    return float4(total, 1.0);
}
