#pragma once

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "Global.h"

#include "DirectXTex.h"
#include "DDS.h"
#include "morton.h"
#include "EncodingDevice.h"
#include "lz4.h"
#include "lz4hc.h"

namespace Texture
{
    enum class Version : uint8_t
    {
        H2016 = 0,
        H2 = 1,
        H3 = 2,
        HMA = 3,
        H2016_A = 4,
        NONE = 255
    };

    enum class Type : uint16_t
    {
        Colour = 0,
        Normal = 1,
        Height = 2,
        Compoundnormal = 3,
        Billboard = 4,
        Unk = 65535
    };

    enum class Format : uint16_t
    {
        R16G16B16A16 = 0x0A,
        R8G8B8A8 = 0x1C,
        R8G8 = 0x34, //Normals. very rarely used. Legacy? Only one such tex in chunk0;
        A8 = 0x42,   //8-bit grayscale uncompressed. Not used on models?? Overlays
        DXT1 = 0x49, //Color maps, 1-bit alpha (mask). Many uses, color, normal, spec, rough maps on models and decals. Also used as masks.
        DXT5 = 0x4F, //Packed color, full alpha. Similar use as DXT5.
        BC4 = 0x52,  //8-bit grayscale. Few or no direct uses on models?
        BC5 = 0x55,  //2-channel normal maps
        BC7 = 0x5A   //high res color + full alpha. Used for pretty much everything...
    };

    struct builtTexture
    {
        uint16_t width;
        uint16_t height;
        uint8_t mipsCount;
        uint32_t mipsSizes[0xE];
        uint32_t compressedSizes[0xE];
        std::vector<char> pixels;
        std::vector<char> compressedPixels;
    };

    namespace HMA
    {
        struct Header
        {
            uint16_t magic;
            Type type;
            uint32_t fileSize;
            uint32_t flags;
            uint16_t width;
            uint16_t height;
            Format format;
            uint8_t mipsCount;
            uint8_t defaultMip;
            uint8_t interpretAs;
            uint8_t dimensions;
            uint16_t mipsInterpolMode;
            uint32_t nIADataSize;
            uint32_t nIADataOffset;
        };

        struct TEXT
        {
            Header header;
            std::vector<char> pixels;
        };

        struct Meta
        {
            Type type;
            uint32_t flags;
            Format format;
            uint8_t interpretAs;
            uint8_t dimensions;
            uint16_t mipsInterpolMode;
        };

        bool readHeader(std::vector<char> textureData, Header &header);

        void Convert(std::vector<char> textureData, std::string outputPath, bool ps4swizzle);
        void Rebuild(std::string tgaPath, std::string outputPath, bool ps4swizzle);
    };

    namespace H2016
    {
        struct Header
        {
            uint16_t magic;
            Type type;
            uint32_t texdIdentifier; // 00 40 00 00 == has/is TEXD | FF FF FF FF == doesn't have TEXD
            uint32_t fileSize;
            uint32_t flags;
            uint16_t width;
            uint16_t height;
            Format format;
            uint8_t mipsCount;
            uint8_t defaultMip;
            uint8_t interpretAs;
            uint8_t dimensions;
            uint8_t mipsInterpolMode;
            uint32_t mipsDataSizes[0xE];
            uint32_t textAtlasSize; // Do not support texture atlases yet, will look into it later
            uint32_t textAtlasOffset;
        };

        struct TEXT
        {
            Header header;
            std::vector<char> textureAtlasData; // For future use
            std::vector<char> pixels;
        };

        struct Meta
        {
            Type type;
            uint32_t flags;
            Format format;
            uint8_t interpretAs;
        };

        bool readHeader(std::vector<char> textureData, Header &header, bool isTEXD);

        void Convert(std::vector<char> textureData, std::string outputPath, bool ps4swizzle, Version portTo, bool isTEXD);
        void Rebuild(std::string tgaPath, std::string outputPath, bool rebuildBoth, bool isTEXD, bool ps4swizzle);
    };

    namespace H2
    {
        struct Header
        {
            uint16_t magic;
            Type type;
            uint32_t fileSize;
            uint32_t flags;
            uint16_t width;
            uint16_t height;
            Format format;
            uint8_t mipsCount;
            uint8_t defaultMip;
            uint32_t texdIdentifier; // 00 40 00 00 == has/is TEXD | FF FF FF FF == doesn't have TEXD
            uint32_t mipsDataSizes[0xE];
            uint32_t mipsDataSizesDup[0xE]; // Duplicate of previous
            uint32_t textAtlasSize;         // Do not support texture atlases yet, will look into it later
            uint32_t textAtlasOffset;
        };

        struct TEXT
        {
            Header header;
            std::vector<char> textureAtlasData; // For future use
            std::vector<char> pixels;
        };

        struct Meta
        {
            Type type;
            uint32_t flags;
            Format format;
        };

        bool readHeader(std::vector<char> textureData, Header &header, bool isTEXD);

        void Convert(std::vector<char> textureData, std::string outputPath, bool ps4swizzle, Version portTo, bool isTEXD);
        void Rebuild(std::string tgaPath, std::string outputPath, bool rebuildBoth, bool isTEXD, bool ps4swizzle);
    };

    namespace H3
    {
        struct Header
        {
            uint16_t magic;
            Type type;
            uint32_t fileSize;
            uint32_t flags;
            uint16_t width;
            uint16_t height;
            Format format;
            uint8_t mipsCount;
            uint8_t defaultMip;
            uint8_t interpretAs = 1;
            uint8_t dimensions;
            uint16_t mipsInterpolMode;
            uint32_t texdMipsSizes[0xE];
            uint32_t texdBlockSizes[0xE]; // Sizes of the compressed blocks of the TEXD
            uint32_t textAtlasSize;
            uint32_t textAtlasOffset; // Do not support texture atlases yet, will look into it later
            uint8_t textScalingData1;
            uint8_t textScalingWidth;
            uint8_t textScalingHeight;
            uint8_t textMipsLevels;
            uint32_t padding; // This brings it to the 0x98 size
        };

        struct TEXT
        {
            Header header;
            std::vector<char> textureAtlasData; // For future use
            std::vector<char> pixels;
        };

        struct TEXD
        {
            std::vector<char> pixels;
        };

        struct Meta
        {
            Type type;
            uint32_t flags;
            Format format;
            bool isCompressed;
            uint8_t textScalingWidth;
            uint8_t textScalingHeight;
        };

        bool readHeader(std::vector<char> textureData, Header &header);

        void Convert(std::vector<char> textData, std::vector<char> texdData, std::string outputPath, bool ps4swizzle, Version portTo, bool hasTEXD, std::string texdOutPath);
        void Rebuild(std::string tgaPath, std::string outputPath, bool rebuildBoth, bool ps4swizzle, std::string texdOutput);
    };

    size_t getScaleFactor(int texdW, int texdH);
    size_t maxMipsCount(size_t width, size_t height);
    size_t getTotalPixelsSize(int width, size_t height, int mipsLevels, DXGI_FORMAT format);
    size_t getPixelBlockSize(Format format);
    DXGI_FORMAT toDxgiFormat(Format format);
    HRESULT createDDS(Format format, uint32_t width, uint32_t height, uint32_t mipCount, std::vector<char> pixels, DirectX::Blob &blob);
    HRESULT compress(builtTexture &texture, DirectX::ScratchImage &mipChain, Format format, bool doCompression);
    HRESULT outputToTGA(DirectX::Blob &blob, Format format, std::filesystem::path outputPath);
    HRESULT import(std::filesystem::path tgaPath, Format format, bool rebuildBoth, bool isTEXD, bool doCompression, builtTexture &TEXT, builtTexture &TEXD);
    std::string versionToString(Version version);
    std::vector<char> PS4swizzle(std::vector<char> &data, Format format, uint16_t width, uint16_t height, bool deswizzle);

    template <typename T>
    T readMetaFile(std::filesystem::path path);
    bool writeFile(void *buffer, size_t size, std::filesystem::path path);

    template <typename T>
    bool writeTexture(T texture, std::filesystem::path path);
}