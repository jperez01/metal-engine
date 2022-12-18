//
//  importUtils.hpp
//  metal_engine
//
//  Created by Juan Perez on 11/26/22.
//

#pragma once

#include <string>
#include <vector>
#include <Metal/Metal.hpp>

#include "model.hpp"

namespace importUtils {
MTL::Texture* textureFromFile(std::string& path, std::string& directory, MTL::Device* device);

MTL::Texture* cubemapFromFile(std::string path, MTL::Device* device);

void addModel(std::string& path, std::vector<Model>& importedModels);
}
