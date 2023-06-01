#pragma once

#include <vector>
#include <string>
#include <cstdint>

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