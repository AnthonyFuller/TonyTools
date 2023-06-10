#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace TonyTools
{
namespace Language
{
    /**
     * @brief Game version enum
     * 
     * 0 and 1 are reserved for future use.
     */
    enum class Version : uint8_t
    {
        H2016 = 2,
        H2,
        H3
    };

    /**
     * @brief Rebuilt data struct containing raw file data and a .meta.json string.
     */
    struct Rebuilt
    {
        std::vector<char> file;
        std::string meta;
    };

    namespace CLNG
    {
        /**
         * @brief Converts a raw CLNG file + .meta.json to a HMLanguages CLNG JSON representation.
         * 
         * @param version The game version the CLNG is from, used for langmap resolution.
         * @param data The raw CLNG data.
         * @param metaJson The .meta.json file (from RPKG Tool) as a string.
         * @param langMap Optional language map, will resolve from version if not supplied. [Default: ""]
         * @return std::string HMLanguages CLNG JSON representation of the input file.
         */
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap = "");
        
        /**
         * @brief Rebuilds a HMLanguages CLNG JSON representation to a raw CLNG file + .meta.json.
         * 
         * @param jsonString The HMLanguages CLNG JSON.
         * @return Rebuilt struct containing the raw file + .meta.json string. 
         */
        Rebuilt Rebuild(std::string jsonString);
    }; // namespace CLNG

    namespace DITL
    {
        /**
         * @brief Converts a raw DITL file + .meta.json to a HMLanguages JSON representation.
         * 
         * @param data The raw DITL data.
         * @param metaJson The .meta.json file (from RPKG Tool) as a string.
         * @return std::string HMLanguages DITL JSON representation of the input file.
         */
        std::string Convert(std::vector<char> data, std::string metaJson);

        /**
         * @brief Rebuilds a HMLanguages DITL JSON representation to a raw DITL file + .meta.json.
         * 
         * @param jsonString The HMLanguages DITL JSON.
         * @return Rebuilt struct containing the raw file + .meta.json string.
         */
        Rebuilt Rebuild(std::string jsonString);
    }; // namespace DITL

    namespace DLGE
    {
        /**
         * @brief Converts a raw DLGE file + .meta.json to a HMLanguages JSON representation.
         * 
         * @param version The game version the DLGE is from, used for langmap resolution and version specific quirks.
         * @param data The raw DLGE data.
         * @param metaJson The .meta.json file (from RPKG Tool) as a string.
         * @param defaultLocale Optional default locale to set the default values for a WavFile. [Default: "en"]
         * @param hexPrecision Optional flag to output random weights as their hex value for higher precision. [Default: false]
         * @param langMap Optional language map, will resolve from version if not supplied. [Default: ""]
         * @return std::string HMLanguages DLGE JSON representation of the input file.
         */
        std::string Convert(Language::Version version,
                            std::vector<char> data,
                            std::string metaJson,
                            std::string defaultLocale = "en",
                            bool hexPrecision = false,
                            std::string langMap = "");

        /**
         * @brief Rebuilds a HMLanguages DLGE JSON representation to a raw DLGE file + .meta.json.
         * 
         * @param version The game version the DLGE is from, used for langmap resolution and version specific quirks.
         * @param jsonString The HMLanguages DLGE JSON.
         * @param defaultLocale Optional default locale for where to use the default values in a WavFile. [Default: "en"]
         * @param langMap Optional language map, will resolve from version if not supplied. [Default: ""]
         * @return Rebuilt struct containing the raw file + .meta.json string.
         */
        Rebuilt Rebuild(Language::Version version, std::string jsonString, std::string defaultLocale = "en", std::string langMap = "");
    } // namespace DLGE

    namespace LOCR
    {
        /**
         * @brief Converts a raw LOCR file + .meta.json to a HMLanguages JSON representation.
         * 
         * @param version The game version the LOCR is from, used for langmap resolution and version specific quirks.
         * @param data The raw LOCR data.
         * @param metaJson The .meta.json file (from RPKG Tool) as a string.
         * @param langMap Optional language map, will resolve from version if not supplied. [Default: ""]
         * @return std::string HMLanguages LOCR JSON representation of the input file.
         */
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap = "");

        /**
         * @brief Rebuilds a HMLanguages LOCR JSON representation to a raw LOCR file + .meta.json.
         * 
         * @param version The game version the LOCR is from, used for version specific quirks.
         * @param jsonString The HMLanguages LOCR JSON.
         * @return Rebuilt struct containing the raw file + .meta.json string.
         */
        Rebuilt Rebuild(Language::Version version, std::string jsonString);
    } // namespace LOCR

    namespace RTLV
    {
        /**
         * @brief Converts a raw RTLV file + .meta.json to a HMLanguages JSON representation.
         * 
         * @param version The game version the RTLV is from, used for ResourceLib.
         * @param data The raw RTLV data.
         * @param metaJson The .meta.json file (from RPKG Tool) as a string.
         * @return std::string HMLanguages RTLV JSON representation of the input file.
         */
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson);

        /**
         * @brief Rebuilds a HMLanguages RTLV JSON representation to a raw RTLV file + .meta.json.
         * 
         * @param version The game version to rebuild the RTLV for, used for langmap resolution and ResourceLib.
         * @param jsonString The HMLanguages RTLV JSON.
         * @param langMap Optional language map, will resolve from version if not supplied. [Default: ""]
         * @return Rebuilt struct containing the raw file + .meta.json string.
         */
        Rebuilt Rebuild(Language::Version version, std::string jsonString, std::string langMap = "");
    } // namespace RTLV
} // namespace Language
} // namespace TonyTools