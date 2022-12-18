#include "importUtils.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <vector>

MTL::Texture* importUtils::textureFromFile(std::string& path, std::string& directory, MTL::Device* device) {
    std::string filename = directory + "/" + path;
    
    int width, height, nrComponents;
    
    stbi_info(filename.c_str(), &width, &height, &nrComponents);
    int numComponents = nrComponents;
    if (nrComponents == 3) numComponents = 4;
    
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, numComponents);
    
    MTL::Texture* texture = nullptr;
    if (data) {
        MTL::PixelFormat format;
        if (nrComponents == 1) format = MTL::PixelFormatR8Unorm;
        else if (nrComponents == 3 || nrComponents == 4) format = MTL::PixelFormatRGBA8Unorm;
        
        MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
        descriptor->setWidth(width);
        descriptor->setHeight(height);
        descriptor->setPixelFormat(format);
        descriptor->setTextureType(MTL::TextureType2D);
        descriptor->setStorageMode(MTL::StorageModeManaged);
        descriptor->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
        
        MTL::Region region = MTL::Region(0, 0, 0, width, height, 1);
        NS::Integer bytesPerRow = 4 * width;
        
        texture = device->newTexture(descriptor);
        texture->replaceRegion(region, 0, data, bytesPerRow);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
    }
    stbi_image_free(data);
    return texture;
}


MTL::Texture* importUtils::cubemapFromFile(std::string path, MTL::Device* device) {
    std::vector<std::string> facePaths = {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "back.jpg",
        "front.jpg"
    };
    
    std::string filename = path + "/" + facePaths[0];
    
    int cubeSize, numComponents;
    stbi_info(filename.c_str(), &cubeSize, nullptr, &numComponents);
    
    if (numComponents == 3) numComponents = 4;
    
    MTL::TextureDescriptor* cubemapDescriptor = MTL::TextureDescriptor::textureCubeDescriptor(MTL::PixelFormatRGBA8Unorm, cubeSize, false);
    cubemapDescriptor->setUsage(MTL::ResourceUsageRead);
    cubemapDescriptor->setStorageMode(MTL::StorageModeManaged);
    
    MTL::Texture* cubeMapTexture = device->newTexture(cubemapDescriptor);
    
    int bytesPerRow = cubeSize * 4;
    int bytesPerImage = cubeSize * bytesPerRow;
    for (int slice = 0; slice < 6; slice++) {
        std::string facePath = path + "/" + facePaths[slice];
        
        int width, height, nrComponents;
        unsigned char* data = stbi_load(facePath.c_str(), &width, &height, &nrComponents, numComponents);
        
        if (data) {
            cubeMapTexture->replaceRegion(MTL::Region::Make2D(0, 0, cubeSize, cubeSize), 0, slice, data, bytesPerRow, bytesPerImage);
        } else {
            std::cout << "Texture failed to load at path: " << facePath << std::endl;
        }
        
        stbi_image_free(data);
    }
    
    return cubeMapTexture;
}

void addModel(MTL::Device* device, std::string& path, std::vector<Model>& importedModels) {
    Model newModel(path, device);
    
    importedModels.push_back(newModel);
}
