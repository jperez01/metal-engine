#pragma once
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

namespace util {
    std::string readFileIntoString(const std::string& fileName);
}