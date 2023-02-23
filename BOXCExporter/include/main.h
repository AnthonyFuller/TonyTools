#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <comdef.h>

#define LOG(x) std::cout << x << std::endl
#define LOG_NO_ENDL(x) std::cout << x
#define LOG_AND_EXIT(x) std::cout << x << std::endl; std::exit(0)
#define LOG_AND_RETURN(x) std::cout << x << std::endl; return
#define LOG_AND_EXIT_NOP(x) std::cout << x << std::endl; std::exit(0)

#include <DirectXTex.h>
#include <DDS.h>
#include "buffer.hpp"
#include "Texture.hpp"
#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Vector3
{
public:
    float x;
    float y;
    float z;

    json toJSON()
	{
		json j = {x, y, z};
        return j;
	}
};

struct Cubemap {
    Vector3 pos;
    std::vector<char> textureData;
};

struct BOXC {
    uint32_t cubemapCount;
    std::vector<Cubemap> cubemaps;
};