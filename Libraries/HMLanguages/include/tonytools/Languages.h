#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "../../src/buffer.hpp"

namespace TonyTools
{
namespace Language
{
    enum class Version : uint8_t
    {
        ALPHA = 0,
        H2016 = 1,
        H2 = 2,
        H3 = 3,
        NONE = 255
    };

    struct Rebuilt
    {
        std::vector<char> file;
        std::string meta;
    };

    namespace CLNG
    {
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap = "");
        Rebuilt Rebuild(Language::Version version, std::string jsonString);
    }; // namespace CLNG

    namespace DITL
    {
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson);
        Rebuilt Rebuild(Language::Version version, std::string jsonString);
    }; // namespace DITL

    namespace DLGE
    {
        struct Metadata
        {
            uint16_t typeIndex; // >> 12 for type -- & 0xFFF for index
            // This is actually a u32 count, then X amount of u32s but our buffer
            // stream reader, when reading a vector, reads a u32 of size first.
            std::vector<uint32_t> SwitchHashes;
        };

        enum class Type : uint8_t
        {
            eDEIT_WavFile = 0x01,
            eDEIT_RandomContainer,
            eDEIT_SwitchContainer,
            eDEIT_SequenceContainer,
            eDEIT_Invalid = 0x15
        };

        class Container
        {
        public:
            uint8_t type;
            uint32_t SwitchGroupHash;
            uint32_t DefaultSwitchHash;
            std::vector<Metadata> metadata;

            Container(buffer &buff);
            Container(uint8_t pType, uint32_t sgh, uint32_t dsh);
            void addMetadata(uint16_t typeIndex, std::vector<uint32_t> entries);
            void write(buffer &buff);
        };

        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string defLocale, bool hexPrecision, std::string langMap = "");
        Rebuilt Rebuild(Language::Version version, std::string jsonString, std::string defaultLocale, std::string langMap);
    } // namespace DLGE

    namespace LOCR
    {
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap = "");
        Rebuilt Rebuild(Language::Version version, std::string jsonString);
    } // namespace LOCR

    namespace RTLV
    {
        std::string Convert(Version version, std::vector<char> data, std::string metaJson);
        Rebuilt Rebuild(Version version, std::string jsonString, std::string langMap);
    } // namespace RTLV
} // namespace Language
} // namespace TonyTools