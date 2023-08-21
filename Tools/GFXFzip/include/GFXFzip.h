#include <vector>
#include <cstdint>
#include <string>

namespace GFXFzip {
    enum class Version : uint8_t
    {
        H2016 = 2,
        H2,
        H3
    };

    struct Rebuilt
    {
        std::string hash;
        std::vector<uint8_t> file;
        std::string meta;
    };

    std::vector<uint8_t> Convert(Version version, std::vector<uint8_t> data, std::vector<uint8_t> metaFileData);
    Rebuilt Rebuild(Version version, std::vector<uint8_t> data);
};
