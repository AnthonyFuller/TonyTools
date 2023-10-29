#include "TonyTools/Languages.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <unordered_map>

#include <ResourceLib_HM2016.h>
#include <ResourceLib_HM2.h>
#include <ResourceLib_HM3.h>
#include <nlohmann/json.hpp>
#include <hash/md5.h>
#include <hash/crc32.h>
#include <tsl/ordered_map.h>

#include "zip.hpp"
#include "bimap.hpp"
#include "buffer.hpp"

using namespace TonyTools;
using json = nlohmann::ordered_json;

#pragma region Utility Functions
bool is_valid_hash(std::string hash)
{
    const std::string valid_chars = "0123456789ABCDEF";

    if (hash.length() != 16)
        return false;

    return std::all_of(hash.begin(), hash.end(), ::isxdigit);
}

std::string computeHash(std::string str)
{
    MD5 md5;
    str = "00" + md5(str).substr(2, 14);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    return str;
}

std::string generateMeta(std::string hash, uint32_t size, std::string type, tsl::ordered_map<std::string, std::string> depends)
{
    json j = {
        {"hash_value", is_valid_hash(hash) ? hash : computeHash(hash)},
        {"hash_offset", 0x10000000},
        {"hash_size", 0x80000000 + size},
        {"hash_resource_type", type},
        {"hash_reference_table_size", (0x9 * depends.size()) + 4},
        {"hash_reference_table_dummy", 0},
        {"hash_size_final", size},
        {"hash_size_in_memory", ULONG_MAX},
        {"hash_size_in_video_memory", ULONG_MAX},
        {"hash_reference_data", json::array()}
    };

    for (const auto &[hash, flag] : depends)
        j.at("hash_reference_data").push_back({{"hash", hash}, {"flag", flag}});

    return j.dump();
}

ResourceConverter* getConverter(Language::Version version, const char *resourceType)
{
    switch (version)
    {
    case Language::Version::H2016:
    {
        if (!HM2016_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2016_GetConverterForResource(resourceType);
    };
    case Language::Version::H2:
    {
        if (!HM2_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2_GetConverterForResource(resourceType);
    };
    case Language::Version::H3:
    {
        if (!HM3_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM3_GetConverterForResource(resourceType);
    };
    default:
        return nullptr;
    }
}

ResourceGenerator* getGenerator(Language::Version version, const char *resourceType)
{
    switch (version)
    {
    case Language::Version::H2016:
    {
        if (!HM2016_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2016_GetGeneratorForResource(resourceType);
    };
    case Language::Version::H2:
    {
        if (!HM2_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2_GetGeneratorForResource(resourceType);
    };
    case Language::Version::H3:
    {
        if (!HM3_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM3_GetGeneratorForResource(resourceType);
    };
    default:
        return nullptr;
    }
}

std::vector<std::string> split(std::string str)
{
    std::regex regex{R"([,]+)"};
    std::sregex_token_iterator it{str.begin(), str.end(), regex, -1};
    return std::vector<std::string>{it, {}};
}

// From https://github.com/glacier-modding/RPKG-Tool/blob/145d8d7d9711d57f1434489706c3d81b2feeed73/src/crypto.cpp#L3-L41
constexpr uint32_t xteaKeys[4] = {0x53527737, 0x7506499E, 0xBD39AEE3, 0xA59E7268};
constexpr uint32_t xteaDelta = 0x9E3779B9;
constexpr uint32_t xteaRounds = 32;

std::string xteaDecrypt(std::vector<char> data)
{
    for (uint32_t i = 0; i < data.size() / 8; i++)
    {
        uint32_t* v0 = (uint32_t*)(data.data() + (i * 8));
        uint32_t* v1 = (uint32_t*)(data.data() + (i * 8) + 4);

        uint32_t sum = xteaDelta * xteaRounds;

        for (uint32_t j = 0; j < xteaRounds; j++)
        {
            *v1 -= (*v0 << 4 ^ *v0 >> 5) + *v0 ^ sum + xteaKeys[sum >> 11 & 3];
            sum -= xteaDelta;
            *v0 -= (*v1 << 4 ^ *v1 >> 5) + *v1 ^ sum + xteaKeys[sum & 3];
        }
    }

    return std::string(data.begin(), std::find(data.begin(), data.end(), '\0'));
}

std::vector<char> xteaEncrypt(std::string str)
{
    std::vector<char> data(str.begin(), str.end());
    uint32_t paddedSize = data.size() + (data.size() % 8 == 0 ? 0 : 8 - (data.size() % 8));
    data.resize(paddedSize, '\0');

    for (uint32_t i = 0; i < paddedSize / 8; i++)
    {
        uint32_t* v0 = (uint32_t*)(data.data() + (i * 8));
        uint32_t* v1 = (uint32_t*)(data.data() + (i * 8) + 4);

        uint32_t sum = 0;

        for (uint32_t j = 0; j < xteaRounds; j++)
        {
            *v0 += (((*v1 << 4) ^ (*v1 >> 5)) + *v1) ^ (sum + xteaKeys[sum & 3]);
            sum += xteaDelta;
            *v1 += (((*v0 << 4) ^ (*v0 >> 5)) + *v0) ^ (sum + xteaKeys[(sum >> 11) & 3]);
        }
    }

    return data;
}

std::vector<char> symmetricEncrypt(std::string str)
{
    std::vector<char> data(str.begin(), str.end());

    for (char &value : data)
    {
        value ^= 226;
        value = (value & 0x81) | (value & 2) << 1 | (value & 4) << 2 | (value & 8) << 3 | (value & 0x10) >> 3 |
           (value & 0x20) >> 2 | (value & 0x40) >> 1;
    }

    return data;
}

std::string symmetricDecrypt(std::vector<char> data)
{
    for (char &value : data)
    {
        value = (value & 1) | (value & 2) << 3 | (value & 4) >> 1 | (value & 8) << 2 | (value & 16) >> 2 | (value & 32) << 1 |
            (value & 64) >> 3 | (value & 128);
        value ^= 226;
    }
    
    return std::string(data.begin(), data.end());
}

std::string getWavName(std::string path, std::string ffxPath, std::string hash)
{
    if (is_valid_hash(path))
        return hash;

    std::regex r{R"([^\/]*(?=\.wav))"};
    std::regex ffxR{R"([^\/]*(?=\.animset))"};
    std::smatch m;
    std::regex_search(path, m, r);

    if (m.size() != 1)
    {
        std::regex_search(ffxPath, m, ffxR);
        if (m.size() != 1)
            return hash;
    }

    CRC32 crc32;

    std::string hashedName = std::format("{:08X}", crc32(m[0]));

    return hashedName == hash ? m[0] : hash;
}

uint32_t hexStringToNum(std::string string)
{
    CRC32 crc32;

    uint32_t num = std::strtoul(string.c_str(), nullptr, 16);
    if (!std::all_of(string.begin(), string.end(), ::isxdigit))
        return crc32(string);

    return num;
}
#pragma endregion

#pragma region Switch and Tag Maps
// Thanks to Notex for compiling this list
const stde::bimap<uint32_t, std::string> TagMap = {};

const stde::bimap<uint32_t, std::string> SwitchMap = {};
#pragma endregion

#pragma region RTLV
std::string Language::RTLV::Convert(Language::Version version, std::vector<char> data, std::string metaJson)
{
    ResourceConverter *converter = getConverter(version, "RTLV");
    if (!converter)
    {
        fprintf(stderr, "[LANG//RTLV] Could not get converter!\n");
        return "";
    }

    JsonString *converted = converter->FromMemoryToJsonString(data.data(), data.size());
    if (!converted)
    {
        fprintf(stderr, "[LANG//RTLV] Could not convert RTLV to ResourceLib JSON!\n");
        return "";
    }

    json j = {
        {"$schema", "https://tonytools.win/schemas/rtlv.schema.json"},
        {"hash", ""},
        {"videos", json::object()},
        {"subtitles", json::object()}
    };

    try
    {
        json jConv = json::parse(converted->JsonData);
        converter->FreeJsonString(converted);

        if (jConv.at("AudioLanguages").size() != jConv.at("VideoRidsPerAudioLanguage").size())
        {
            fprintf(stderr, "[LANG//RTLV] Mismatch in languages and resource IDs in RL JSON!\n");
            return "";
        }

        for (const auto &[lang, id] : c9::zip(jConv.at("AudioLanguages"), jConv.at("VideoRidsPerAudioLanguage")))
            j.at("videos").push_back({lang, std::format("{:08X}{:08X}", id.at("m_IDHigh").get<uint32_t>(), id.at("m_IDLow").get<uint32_t>())});

        if (jConv.at("SubtitleLanguages").size() != jConv.at("SubtitleMarkupsPerLanguage").size())
        {
            fprintf(stderr, "[LANG//RTLV] Mismatch in subtitle languages and content in RL JSON!\n");
            return "";
        }

        for (const auto &[lang, text] : c9::zip(jConv.at("SubtitleLanguages"), jConv.at("SubtitleMarkupsPerLanguage")))
            j.at("subtitles").push_back({lang, text});

        json meta = json::parse(metaJson);
        j.at("hash") = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    }
    catch (const json::exception& err)
    {
        if (converted)
            converter->FreeJsonString(converted);

        fprintf(stderr, "[LANG//RTLV] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return "";
}

Language::Rebuilt Language::RTLV::Rebuild(Language::Version version, std::string jsonString, std::string langMap)
{
    ResourceGenerator *generator = getGenerator(version, "RTLV");
    if (!generator)
    {
        fprintf(stderr, "[LANG//RTLV] Could not get generator!\n");
        return {};
    }

    Language::Rebuilt out{};
    tsl::ordered_map<std::string, std::string> depends{};

    std::unordered_map<std::string, uint32_t> languages;
    if (!langMap.empty())
    {
        std::vector<std::string> langs = split(langMap);
        for (int i = 0; i < langs.size(); i++)
            languages[langs.at(i)] = i;
    }
    else if (version == Version::H3)
        languages = {{"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"cn", 7}, {"tc", 8}, {"jp", 9}};
    else
        languages = {{"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"mx", 7}, {"br", 8}, {"pl", 9}, {"cn", 10}, {"jp", 11}, {"tc", 12}};

    try
    {
        json jSrc = json::parse(jsonString);

        if (jSrc.at("videos").size() < 1 || jSrc.at("subtitles").size() < 1)
        {
            fprintf(stderr, "[LANG//RTLV] Videos and/or subtitles object is empty!\n");
            return {};
        }

        // The langmap property overrides any argument passed languages maps.
        // This property ensures easy compat with tools like SMF.
        if (jSrc.contains("langmap"))
        {
            languages.clear();
            std::vector<std::string> langs = split(jSrc.at("langmap").get<std::string>());
            for (int i = 0; i < langs.size(); i++)
                languages[langs.at(i)] = i;
        }

        json j = {
            {"AudioLanguages", {}},
            {"VideoRidsPerAudioLanguage", {}},
            {"SubtitleLanguages", {}},
            {"SubtitleMarkupsPerLanguage", {}}
        };

        for (const auto &[lang, video] : jSrc.at("videos").items())
        {
            if (!languages.contains(lang))
            {
                fprintf(stderr, "[LANG//RTLV] Language map does not contain language \"%s\".\n", lang.c_str());
                return {};
            }

            j.at("AudioLanguages").push_back(lang);

            std::string vidHash = video.get<std::string>();
            if (!is_valid_hash(vidHash))
                vidHash = computeHash(vidHash);

            uint64_t id = std::strtoull(vidHash.c_str(), nullptr, 16);

            j.at("VideoRidsPerAudioLanguage").push_back(json::object({{"m_IDHigh", id >> 32}, {"m_IDLow", id & ULONG_MAX}}));

            depends[vidHash] = std::format("{:2X}", 0x80 + languages[lang]);
        }

        for (const auto &[lang, text] : jSrc.at("subtitles").items())
        {
            j.at("SubtitleLanguages").push_back(lang);
            j.at("SubtitleMarkupsPerLanguage").push_back(text);
        }

        std::string rlJson = j.dump();
        ResourceMem *generated = generator->FromJsonStringToResourceMem(rlJson.c_str(), rlJson.size(), false);
        if (!generated)
        {
            fprintf(stderr, "[LANG//RTLV] Could not convert ResourceLib JSON to RTLV!\n");
            return {};
        }

        out.file.resize(generated->DataSize);
        std::memcpy(out.file.data(), generated->ResourceData, generated->DataSize);
        generator->FreeResourceMem(generated);

        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "RTLV", depends);

        return out;
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//RTLV] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return {};
}
#pragma endregion

#pragma region LOCR
std::string Language::LOCR::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap, bool symmetric)
{
    buffer buff(data);

    bool isLOCRv2 = false;
    if (version != Version::H2016)
    {
        buff.read<uint8_t>();
        isLOCRv2 = true;
    }

    json j = {
        {"$schema", "https://tonytools.win/schemas/locr.schema.json"},
        {"hash", ""},
        {"symmetric", true},
        {"languages", json::object()}
    };

    if (!symmetric || version != Version::H2016)
        j.erase("symmetric");

    uint32_t numLanguages = (buff.read<uint32_t>() - isLOCRv2) / 4;
    buff.index -= 4;
    std::vector<std::string> languages = {"xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc"};;
    if (!langMap.empty())
        languages = split(langMap);
    else if (version == Version::H3)
        languages = {"xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp"};

    if (numLanguages > languages.size())
    {
        fprintf(stderr, "[LANG//LOCR] Language map is smaller than the number of languages in the file!\n");
        return "";
    }

    size_t oldIndex = buff.index;
    for (int i = 0; i < numLanguages; i++)
    {
        j.at("languages").push_back({languages.at(i), json::object()});

        uint32_t oldOffset = buff.index;
        uint32_t offset;
        buff.index = oldIndex;
        if ((offset = buff.read<uint32_t>()) == ULONG_MAX)
        {
            buff.index = oldOffset;
            oldIndex += 4;
            continue;
        }
        buff.index = offset;
        oldIndex += 4;

        uint32_t numStrings = buff.read<uint32_t>();
        for (int k = 0; k < numStrings; k++)
        {
            uint32_t hash = buff.read<uint32_t>();
            std::string str = (symmetric && version == Version::H2016)
                                ? symmetricDecrypt(buff.read<std::vector<char>>())
                                : xteaDecrypt(buff.read<std::vector<char>>());
            buff.index += 1;

            j.at("languages").at(languages.at(i)).push_back({std::format("{:08X}", hash), str});
        }
    }

    if (buff.index != buff.size())
    {
        fprintf(stderr, "[LANG//LOCR] Did not read to end of file! Report this!\n");
        return "";
    }

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//LOCR] JSON error:\n"
                        "\t%s%s\n", err.what(),
                        version == Version::H2016
                        ? "\nIf this is an older H2016 LOCR file, this may be due to a symmetric cipher being used." : "");
    }

    return "";
}

Language::Rebuilt Language::LOCR::Rebuild(Language::Version version, std::string jsonString, bool symmetric)
{
    Language::Rebuilt out{};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        if (!jSrc["symmetric"].is_null() && jSrc["symmetric"].get<bool>() && version == Version::H2016)
            symmetric = true;

        if (version != Version::H2016)
            buff.write<char>('\0');

        uint32_t curOffset = buff.index;
        buff.insert(jSrc.at("languages").size() * 4);

        for (const auto &[lang, strings] : jSrc.at("languages").items())
        {
            if (!strings.size())
            {
                uint32_t temp = buff.index;
                buff.index = curOffset;
                buff.write<uint32_t>(ULONG_MAX);
                curOffset += 4;
                buff.index = temp;
                continue;
            }

            uint32_t temp = buff.index;
            buff.index = curOffset;
            buff.write<uint32_t>(temp);
            curOffset += 4;
            buff.index = temp;

            buff.write<uint32_t>(strings.size());
            for (const auto &[strHash, string] : strings.items())
            {
                buff.write<uint32_t>(hexStringToNum(strHash));
                buff.write<std::vector<char>>((symmetric && version == Version::H2016)
                                                ? symmetricEncrypt(string.get<std::string>())
                                                : xteaEncrypt(string.get<std::string>())
                                            );
                buff.write<char>('\0');
            }
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "LOCR", {});

        return out;
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//LOCR] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return {};
}
#pragma endregion

#pragma region DITL
std::string Language::DITL::Convert(std::vector<char> data, std::string metaJson)
{
    buffer buff(data);

    json j = {
        {"$schema", "https://tonytools.win/schemas/ditl.schema.json"},
        {"hash", ""},
        {"soundtags", json::object()}
    };

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        uint32_t count = buff.read<uint32_t>();
        for (uint32_t i = 0; i < count; i++)
        {
            std::string depend = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");
            uint32_t soundtagHash = buff.read<uint32_t>();

            j.at("soundtags").push_back({
                TagMap.has_key(soundtagHash) ? TagMap.get_value(soundtagHash) : std::format("{:08X}", soundtagHash),
                depend
            });
        }

        // Sanity check
        if (buff.index != buff.size())
        {
            fprintf(stderr, "[LANG//DITL] Did not read to the end of the file! Report this!\n");
            return "";
        }

        return j.dump();
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//DITL] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return "";
}

Language::Rebuilt Language::DITL::Rebuild(std::string jsonString)
{
    Language::Rebuilt out{};
    tsl::ordered_map<std::string, std::string> depends{};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        buff.write<uint32_t>(jSrc.at("soundtags").size());

        for (const auto &[tagName, hash] : jSrc.at("soundtags").items())
        {
            if (depends.contains(hash))
                buff.write<uint32_t>(depends.find(hash) - depends.begin());
            else
            {
                buff.write<uint32_t>(depends.size());
                depends[hash] = "1F";
            }

            buff.write<uint32_t>(TagMap.has_value(tagName) ? TagMap.get_key(tagName) : hexStringToNum(tagName));
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "DITL", depends);

        return out;
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//DITL] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return {};
}
#pragma endregion

#pragma region CLNG
std::string Language::CLNG::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap)
{
    buffer buff(data);

    json j = {
        {"$schema", "https://tonytools.win/schemas/clng.schema.json"},
        {"hash", ""},
        {"languages", json::object()}
    };

    std::vector<std::string> languages = {"xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc"};

    if (!langMap.empty())
        languages = split(langMap);
    else if (version == Version::H3)
        languages = {"xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp"};

    uint32_t i = 0;
    while (buff.index != buff.size())
    {
        if (i >= languages.size())
        {
            fprintf(stderr, "[LANG//CLNG] Language map is smaller than the number of languages in the file!\n");
            return "";
        }

        j.at("languages").push_back({languages.at(i++), buff.read<bool>()});
    }

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//CLNG] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return "";
}

Language::Rebuilt Language::CLNG::Rebuild(std::string jsonString)
{
    Language::Rebuilt out{};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        for (const auto &[language, value] : jSrc.at("languages").items())
            buff.write<bool>(value.get<bool>());

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash").get<std::string>(), out.file.size(), "CLNG", {});

        return out;
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//CLNG] JSON error:\n"
                        "\t%s\n", err.what());
    }
    
    return {};
    
}
#pragma endregion

#pragma region DLGE
struct DLGE_Metadata
{
    uint16_t typeIndex; // >> 12 for type -- & 0xFFF for index
    // This is actually a u32 count, then X amount of u32s but our buffer
    // stream reader, when reading a vector, reads a u32 of size first.
    std::vector<uint32_t> SwitchHashes;
};

enum class DLGE_Type : uint8_t
{
    eDEIT_WavFile = 0x01,
    eDEIT_RandomContainer,
    eDEIT_SwitchContainer,
    eDEIT_SequenceContainer,
    eDEIT_Invalid = 0x15
};

NLOHMANN_JSON_SERIALIZE_ENUM(DLGE_Type, {
    {DLGE_Type::eDEIT_Invalid, "Invalid"},
    {DLGE_Type::eDEIT_WavFile, "WavFile"},
    {DLGE_Type::eDEIT_RandomContainer, "Random"},
    {DLGE_Type::eDEIT_SwitchContainer, "Switch"},
    {DLGE_Type::eDEIT_SequenceContainer, "Sequence"}
});

class DLGE_Container
{
public:
    uint8_t type;
    uint32_t SwitchGroupHash;
    uint32_t DefaultSwitchHash;
    std::vector<DLGE_Metadata> metadata;

    DLGE_Container(buffer &buff) 
    {
        type = buff.read<uint8_t>();
        SwitchGroupHash = buff.read<uint32_t>();
        DefaultSwitchHash = buff.read<uint32_t>();

        uint32_t count = buff.read<uint32_t>();
        DLGE_Metadata data;
        for (uint32_t i = 0; i < count; i++)
        {
            data = {
                buff.read<uint16_t>(),
                buff.read<std::vector<uint32_t>>()
            };

            metadata.push_back(data);
        }
    };

    DLGE_Container(uint8_t pType, uint32_t sgh, uint32_t dsh)
    {
        type = pType;
        SwitchGroupHash = sgh;
        DefaultSwitchHash = dsh;
        metadata = {};
    };

    void addMetadata(uint16_t typeIndex, std::vector<uint32_t> entries)
    {
        metadata.push_back({
            typeIndex,
            entries
        });
    };

    void write(buffer &buff)
    {
        buff.write<uint8_t>(type);
        buff.write<uint32_t>(SwitchGroupHash);
        buff.write<uint32_t>(DefaultSwitchHash);
        
        buff.write<uint32_t>(metadata.size());
        for (const DLGE_Metadata &metadata : metadata)
        {
            buff.write<uint16_t>(metadata.typeIndex);
            buff.write<std::vector<uint32_t>>(metadata.SwitchHashes);
        }
    };
};

std::string Language::DLGE::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string defaultLocale, bool hexPrecision, std::string langMap)
{
    buffer buff(data);

    json j = {
        {"$schema", "https://tonytools.win/schemas/dlge.schema.json"},
        {"hash", ""},
        {"DITL", ""},
        {"CLNG", ""}
    };

    // These are H2 languages by default.
    std::vector<std::string> languages = {"xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc"};
    if (!langMap.empty())
    {
        languages = split(langMap);
        j.push_back({"langmap", langMap});
    }
    else if (version == Version::H2016)
        // Late versions of H2016 share the same langmap as H2, but without tc, so we remove it.
        languages.pop_back();
    else if (version == Version::H3)
        languages = {"xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp"};

    j.push_back({"rootContainer", nullptr});

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");
        j["DITL"] = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");
        j["CLNG"] = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");

        // We setup these maps to store the various types of containers and the latest index for final construction later.
        std::map<uint8_t, tsl::ordered_map<uint32_t, json>> containerMap = {
            {1, {}},
            {2, {}},
            {3, {}},
            {4, {}}
        };
        std::map<uint8_t, uint32_t> indexMap = {
            {1, 0},
            {2, 0},
            {3, 0},
            {4, 0}
        };

        // Weirdly, sequences reference by some "global id" for certain types so we store this here.
        uint32_t globalIndex = -1;
        std::unordered_map<uint32_t, uint32_t> globalMap = {};
        
        // Read everything but the root typedIndex as we read that above.
        while (buff.index != (buff.size() - 2))
        {
            switch (buff.peek<uint8_t>())
            {
            case 0x01: // eDEIT_WavFile
            {
                buff.index += 1; // We don't need to record the type.
                uint32_t soundTagHash = buff.read<uint32_t>();
                uint32_t wavNameHash = buff.read<uint32_t>();

                // H2016 has this at the start of every language in a wav file container, we check this later.
                if (version != Version::H2016)
                    buff.read<uint32_t>();

                json wav = json::object({
                    {"type", DLGE_Type::eDEIT_WavFile},
                    {"wavName", std::format("{:08X}", wavNameHash)},
                    {"cases", nullptr},
                    {"weight", nullptr},
                    {"soundtag", TagMap.has_key(soundTagHash) ? TagMap.get_value(soundTagHash) : std::format("{:08X}", soundTagHash)},
                    {"defaultWav", nullptr},
                    {"defaultFfx", nullptr},
                    {"languages", json::object()}
                });

                for (std::string const &language : languages)
                {
                    if (version == Version::H2016)
                        buff.read<uint32_t>();

                    uint32_t wavIndex = buff.read<uint32_t>(); // WWES/WWEM depend index
                    uint32_t ffxIndex = buff.read<uint32_t>(); // FaceFX depend index

                    json subtitleJson = nullptr;

                    if (wavIndex != ULONG_MAX && ffxIndex != ULONG_MAX)
                    {
                        if (language == defaultLocale)
                        {
                            wav.at("defaultWav") = meta.at("hash_reference_data").at(wavIndex).at("hash");
                            wav.at("defaultFfx") = meta.at("hash_reference_data").at(ffxIndex).at("hash");

                            // As we are most likely to have the english (default locale unless specified) hash, we get the wav hash from here.
                            wav.at("wavName") = getWavName(wav.at("defaultWav"), wav.at("defaultFfx"), std::format("{:08X}", wavNameHash));
                        }
                        else
                        {
                            subtitleJson = json::object({
                                {"wav", meta.at("hash_reference_data").at(wavIndex).at("hash")},
                                {"ffx", meta.at("hash_reference_data").at(ffxIndex).at("hash")}
                            });
                        }
                    }

                    if (buff.peek<uint32_t>() != 0)
                        if (subtitleJson.is_null())
                            subtitleJson = xteaDecrypt(buff.read<std::vector<char>>());
                        else
                            subtitleJson.push_back({"subtitle", xteaDecrypt(buff.read<std::vector<char>>())});
                    else
                        // We do this as we only peeked the size.
                        buff.index += 4;

                    if (!subtitleJson.is_null())
                        wav.at("languages").push_back({language, subtitleJson});
                }

                containerMap.at(0x01)[indexMap.at(0x01)++] = wav;
                break;
            }
            case 0x02: // eDEIT_RandomContainer
            {
                DLGE_Container container(buff);

                json cjson = json::object({
                    {"type", DLGE_Type::eDEIT_RandomContainer},
                    {"cases", nullptr},
                    {"containers", json::array()}
                });

                for (const DLGE_Metadata &metadata : container.metadata)
                {
                    // Random containers will ONLY EVER CONTAIN references to wav files.
                    // They will also only ever contain one "SwitchHashes" entry with the weight. 
                    // This has been verified across all games. This also makes sense when considering
                    // the purpose of the different containers. It makes no sense for a switch group or sequence to be randomised.

                    uint8_t type = metadata.typeIndex >> 12;
                    uint32_t index = metadata.typeIndex & 0xFFF;

                    // Referencing anything other than WavFiles in a random container is illogical.
                    if (type != 0x01) {
                        fprintf(stderr, "[LANG//DLGE] Bad random container reference [0x%02X].\n", type);
                        return "";
                    }

                    // Remove the switch property as it will be unused.
                    containerMap.at(type).at(index).erase("cases");

                    // Cannot use ternary here due to conflicting types.
                    if (hexPrecision) 
                        containerMap.at(type).at(index).at("weight") = std::format("{:06X}", metadata.SwitchHashes[0]);
                    else
                        containerMap.at(type).at(index).at("weight") = (double)metadata.SwitchHashes[0] / (double)0xFFFFFF;

                    cjson.at("containers").push_back(containerMap.at(type).at(index));
                    containerMap.at(type).erase(index);
                }

                containerMap.at(0x02)[indexMap.at(0x02)++] = cjson;
                globalMap[++globalIndex] = indexMap.at(0x02) - 1;
                break;
            }
            case 0x03: // eDEIT_SwitchContainer
            {
                DLGE_Container container(buff);

                json cjson = json::object({
                    {"type", DLGE_Type::eDEIT_SwitchContainer},
                    {
                        "switchKey", SwitchMap.has_key(container.SwitchGroupHash)
                            ? SwitchMap.get_value(container.SwitchGroupHash)
                            : std::format("{:08X}", container.SwitchGroupHash)
                    },
                    {
                        "default", SwitchMap.has_key(container.DefaultSwitchHash)
                            ? SwitchMap.get_value(container.DefaultSwitchHash)
                            : std::format("{:08X}", container.DefaultSwitchHash)
                    },
                    {"containers", json::array()}
                });

                for (const DLGE_Metadata &metadata : container.metadata) {
                    // Switch containers will ONLY EVER CONTAIN references to random containers. And there will only ever be 1 per DLGE.
                    // But, they may contain more than one entry (or no entries) in the "SwitchHashes" array.
                    // This has been verified across all games. This, again, makes sense when considering the purposes of each container.
                    // But, we allow WavFile references in HMLT as they make sense, but currently it's unknown if the game allows for this.

                    uint8_t type = metadata.typeIndex >> 12;
                    uint32_t index = metadata.typeIndex & 0xFFF;

                    // Referencing anything other than WavFiles and random containers in a switch container is illogical.
                    if (type != 0x01 && type != 0x02) {
                        fprintf(stderr, "[LANG//DLGE] Bad switch container reference [0x%02X].\n", type);
                        return "";
                    }

                    // Remove the weight value as it isn't used.
                    containerMap.at(type).at(index).erase("weight");

                    json caseArray = json::array();
                    for (auto &hash : metadata.SwitchHashes)
                        caseArray.push_back(SwitchMap.has_key(hash) ? SwitchMap.get_value(hash) : std::format("{:08X}", hash));

                    containerMap.at(type).at(index).at("cases") = caseArray;
                    cjson.at("containers").push_back(containerMap.at(type).at(index));
                    containerMap.at(type).erase(index);
                }

                containerMap.at(0x03)[indexMap.at(0x03)++] = cjson;
                globalMap[++globalIndex] = indexMap.at(0x03) - 1;
                break;
            }
            case 0x04: // eDEIT_SequenceContainer
            {
                // Sequence containers can contain any of the containers apart from sequence containers of course.
                // Unsure if this is a hard limitation, or if they've just not used any.
                // Further testing required. (Although if it is a limitation, this is logical).

                DLGE_Container container(buff);

                json cjson = json::object({
                    {"type", DLGE_Type::eDEIT_SequenceContainer},
                    {"containers", json::array()}
                });
                
                for (const DLGE_Metadata &metadata : container.metadata)
                {
                    uint8_t type = metadata.typeIndex >> 12;
                    uint32_t index = (type == 0x02 || type == 0x03) ? globalMap.at(metadata.typeIndex & 0xFFF) : metadata.typeIndex & 0xFFF;

                    if (type == 0x04) {
                        fprintf(stderr, "[LANG//DLGE] A sequence container cannot contain a sequence.\n");
                        return "";
                    }

                    // Remove cases and weight property as it won't be used.
                    switch(type) {
                        case 0x01:
                            containerMap.at(type).at(index).erase("weight");
                        case 0x02:
                            containerMap.at(type).at(index).erase("cases");
                    }

                    // We can do this as there is only one switch container per DLGE.
                    cjson.at("containers").push_back(type == 0x03 ? (--containerMap.at(type).end())->second : containerMap.at(type).at(index));
                    if (type == 0x03)
                        containerMap.at(type).clear();
                    else
                        containerMap.at(type).erase(index);
                }

                containerMap.at(0x04)[indexMap.at(0x04)++] = cjson;
                break;
            }
            case 0x15: // eDEIT_Invalid
            {
                fprintf(stderr, "[LANG//DLGE] Invalid section found. Report this!\n");
                return "";
            }
            default: // Just in case
            {
                fprintf(stderr, "[LANG//DLGE] Unknown section found [0x%02X]. Report this!\n", buff.read<uint8_t>());
                return "";
            }
            }
        }
        
        bool set = false;
        for (const auto [type, typedContainerMap] : containerMap) {
            if(!typedContainerMap.size()) continue;
            if(typedContainerMap.size() != 1 || set)
            {
                fprintf(stderr, "[LANG//DLGE] More than one container left over. Report this!\n");
                return "";
            }

            j.at("rootContainer") = typedContainerMap.rbegin()->second;

            switch (type) {
                case 0x01:
                    j.at("rootContainer").erase("weight");
                case 0x02:
                    j.at("rootContainer").erase("cases");
            }
            set = true;
        }

        if (!set)
        {
            fprintf(stderr, "[LANG//DLGE] No root container found. Report this!\n");
            return "";
        }

        return j.dump();
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//DLGE] JSON error:\n"
                        "\t%s\n", err.what());
        fprintf(stdout, "This could be due to your language maps not matching up to the number of languages in the file (older file?).\n");
    }

    return "";
}

// Avoids code duplication
void addDepend(
    buffer &buff,
    std::string hash,
    std::string flag,
    tsl::ordered_map<std::string, std::string> &depends
)
{
    if (depends.contains(hash))
        buff.write<uint32_t>(depends.find(hash) - depends.begin());
    else
    {
        buff.write<uint32_t>(depends.size());
        depends[hash] = flag;
    }
}

bool processContainer(
    Language::Version version,
    buffer &buff,
    json container,
    std::unordered_map<uint32_t, uint32_t> &indexMap,
    std::vector<std::pair<std::string, uint32_t>> languages,
    tsl::ordered_map<std::string, std::string> &depends,
    std::string defLocale
)
{
    DLGE_Type type = container.at("type").get<DLGE_Type>();

    switch (type) {
        case DLGE_Type::eDEIT_WavFile: {
            buff.write<uint8_t>(0x01);

            std::string soundTag = container.at("soundtag").get<std::string>();
            buff.write<uint32_t>(TagMap.has_value(soundTag) ? TagMap.get_key(soundTag) : hexStringToNum(soundTag));
            buff.write<uint32_t>(hexStringToNum(container.at("wavName").get<std::string>()));

            if (version != Language::Version::H2016)
                buff.write<uint32_t>(0x00);

            for (const auto &[language, index] : languages)
            {
                if (version == Language::Version::H2016)
                    buff.write<uint32_t>(0x00);

                if (defLocale == language)
                {
                    if (!container.at("defaultWav").is_null() && !container.at("defaultFfx").is_null())
                    {
                        addDepend(
                            buff,
                            container.at("defaultWav").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );

                        addDepend(
                            buff,
                            container.at("defaultFfx").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );
                    }
                    else
                        buff.write<uint64_t>(ULLONG_MAX);

                    if (container.at("languages").contains(language))
                        if (container.at("languages").at(language).size() == 0)
                            buff.write<uint32_t>(0x00);
                        else
                            buff.write<std::vector<char>>(xteaEncrypt(container.at("languages").at(language).get<std::string>()));
                    else
                        buff.write<uint32_t>(0x00);
                }
                else
                {
                    if (!container.at("languages").contains(language))
                    {
                        buff.write<uint64_t>(ULLONG_MAX);
                        buff.write<uint32_t>(0x00);

                        continue;
                    }

                    if (container.at("languages").at(language).is_object())
                    {
                        // This language has wav and ffx (and possibly subtitles).
                        addDepend(
                            buff,
                            container.at("languages").at(language).at("wav").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );

                        addDepend(
                            buff,
                            container.at("languages").at(language).at("ffx").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );

                        if (container.at("languages").at(language).contains("subtitle"))
                            buff.write<std::vector<char>>(xteaEncrypt(container.at("languages").at(language).at("subtitle").get<std::string>()));
                        else
                            buff.write<uint32_t>(0x00);

                        continue;
                    }

                    buff.write<uint64_t>(ULLONG_MAX);

                    if (container.at("languages").at(language).size() == 0)
                        buff.write<uint32_t>(0x00);
                    else
                        buff.write<std::vector<char>>(xteaEncrypt(container.at("languages").at(language).get<std::string>()));
                }
            }

            indexMap.at(0x01)++;
            break;
        }
        case DLGE_Type::eDEIT_RandomContainer: {
            DLGE_Container rawContainer(0x02, 0, 0);

            // We need to write the wav files first.
            for (const json &childContainer : container.at("containers"))
            {
                if (childContainer.at("type").get<DLGE_Type>() != DLGE_Type::eDEIT_WavFile)
                {
                    fprintf(stderr, "[LANG//DLGE] Invalid type found in Random container!\n");
                    return false;
                }

                if (!childContainer.contains("weight"))
                {
                    fprintf(stderr, "[LANG//DLGE] Missing weight in Random container child.\n");
                    return false;
                }

                if (!processContainer(
                    version,
                    buff,
                    childContainer,
                    indexMap,
                    languages,
                    depends,
                    defLocale
                ))
                {
                    fprintf(stderr, "[LANG//DLGE] Failed to process a Random container child.\n");
                    return false;
                }
            
                uint32_t weight;

                if (childContainer.at("weight").is_string())
                {
                    std::string weightStr = childContainer.at("weight").get<std::string>();
                    // Hex precision was enabled on convert.
                    weight = std::strtoul(childContainer.at("weight").get<std::string>().c_str(), NULL, 16);
                    if (!std::all_of(weightStr.begin(), weightStr.end(), ::isxdigit))
                    {
                        fprintf(stderr, "[LANG//DLGE] Invalid weight found in Random container child.\n");
                        return false;
                    }
                }
                else
                    // It must be a double.
                    weight = round(childContainer.at("weight").get<double>() * 0xFFFFFF);

                // We only ever have WavFiles in a random container.
                rawContainer.addMetadata(
                    (0x01 << 12) | (indexMap.at(0x01) & 0xFFF),
                    std::vector<uint32_t>({
                        weight
                    })
                );
            }

            rawContainer.write(buff);
            indexMap.at(0x00)++;
            indexMap.at(0x02)++;
            break;
        }
        case DLGE_Type::eDEIT_SwitchContainer: {
            // This allows us to ensure that there's only one switch container per DLGE.
            if (indexMap.at(0x03) != -1)
            {
                fprintf(stderr, "[LANG//DLGE] Multiple Switch containers found in DLGE!\n");
                return false;
            }

            if (!container.contains("switchKey") || !container.contains("default"))
            {
                fprintf(stderr, "[LANG//DLGE] Switch container is missing \"switchKey\" or \"default\" property.\n");
                return false;
            }

            std::string switchKey = container.at("switchKey").get<std::string>();
            std::string defGroup = container.at("default").get<std::string>();
            DLGE_Container rawContainer(
                0x03,
                SwitchMap.has_value(switchKey) ? SwitchMap.get_key(switchKey) : hexStringToNum(switchKey),
                SwitchMap.has_value(defGroup) ? SwitchMap.get_key(defGroup) : hexStringToNum(defGroup)
            );

            for (const json &childContainer : container.at("containers"))
            {
                DLGE_Type cType = childContainer.at("type").get<DLGE_Type>();

                if (cType != DLGE_Type::eDEIT_WavFile && cType != DLGE_Type::eDEIT_RandomContainer)
                {
                    fprintf(stderr, "[LANG//DLGE] Invalid type found in Switch container.\n");
                    return false;
                }

                if (!childContainer.contains("cases"))
                {
                    fprintf(stderr, "[LANG//DLGE] Cases array missing from Switch container child.\n");
                    return false;
                }

                if (!processContainer(
                    version,
                    buff,
                    childContainer,
                    indexMap,
                    languages,
                    depends,
                    defLocale
                ))
                {
                    fprintf(stderr, "[LANG//DLGE] Failed to process a Switch container child.\n");
                    return false;
                }

                std::vector<uint32_t> switchCases{};

                for (const json &sCase : childContainer.at("cases"))
                {
                    std::string caseStr = sCase.get<std::string>();
                    switchCases.push_back(
                        SwitchMap.has_value(caseStr) ? SwitchMap.get_key(caseStr) : hexStringToNum(caseStr)
                    );
                }

                rawContainer.addMetadata(
                    ((uint8_t)cType << 12) | (indexMap.at((uint8_t)cType) & 0xFFF),
                    switchCases
                );

                indexMap.at(0x03)++;
            }

            rawContainer.write(buff);
            indexMap.at(0x00)++;
            indexMap.at(0x03)++;
            break;
        }
        case DLGE_Type::eDEIT_SequenceContainer: {
            // This allows us to ensure that there's only one sequence container per DLGE.
            if (indexMap.at(0x04) != -1)
            {
                fprintf(stderr, "[LANG//DLGE] Multiple Sequence containers found in DLGE!\n");
                return false;
            }

            DLGE_Container rawContainer(0x04, 0, 0);

            for (const json &childContainer : container.at("containers"))
            {
                DLGE_Type cType = childContainer.at("type").get<DLGE_Type>();

                if (!processContainer(
                    version,
                    buff,
                    childContainer,
                    indexMap,
                    languages,
                    depends,
                    defLocale
                ))
                {
                    fprintf(stderr, "[LANG//DLGE] Failed to process a Sequence container child.\n");
                    return false;
                }

                rawContainer.addMetadata(
                    ((uint8_t)cType << 12) | (indexMap.at((uint8_t)cType == 0x01 ? 0x01 : 0x00) & 0xFFF),
                    {}
                );

                indexMap.at(0x04)++;
            }

            rawContainer.write(buff);
            indexMap.at(0x00)++;
            indexMap.at(0x04)++;
            break;
        }
        case DLGE_Type::eDEIT_Invalid: {
            fprintf(stderr, "[LANG//DLGE] Invalid type found in JSON.\n");
            return false;
        }
    }

    if (container.contains("isRoot"))
        buff.write<uint16_t>(((uint8_t)type << 12) | (indexMap.at((uint8_t)type == 0x01 ? 0x01 : 0x00) & 0xFFF));

    return true;
}

Language::Rebuilt Language::DLGE::Rebuild(Language::Version version, std::string jsonString, std::string defaultLocale, std::string langMap)
{
    Language::Rebuilt out{};
    tsl::ordered_map<std::string, std::string> depends{};

    // We require it to be ordered. These are, like usual, the H2 languages.
    std::vector<std::pair<std::string, uint32_t>> languages = {
        {"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"mx", 7}, {"br", 8}, {"pl", 9}, {"cn", 10}, {"jp", 11}, {"tc", 12}
    };
    if (!langMap.empty())
    {
        languages.clear();
        std::vector<std::string> langs = split(langMap);
        for (int i = 0; i < langs.size(); i++)
            languages.push_back({ langs.at(i), i });
    }
    else if (version == Version::H2016)
        // Late versions of H2016 share the same langmap as H2, but without tc, so we remove it.
        languages.pop_back();
    else if (version == Version::H3)
        languages = {{"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"cn", 7}, {"tc", 8}, {"jp", 9}};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        // The langmap property overrides any argument passed languages maps.
        // This property ensures easy compat with tools like SMF.
        if (jSrc.contains("langmap"))
        {
            languages.clear();
            std::vector<std::string> langs = split(jSrc.at("langmap").get<std::string>());
            for (int i = 0; i < langs.size(); i++)
                languages.push_back({ langs.at(i), i });
        }
        
        buff.write<uint32_t>(0x00);
        depends[jSrc.at("DITL").get<std::string>()] = "1F";
        buff.write<uint32_t>(0x01);
        depends[jSrc.at("CLNG").get<std::string>()] = "1F";

        // So the processContainer function knows what the root container is.
        jSrc.at("rootContainer").push_back({ "isRoot", true });

        // 0 is the "global" index
        std::unordered_map<uint32_t, uint32_t> indexMap = {
            {0, -1},
            {1, -1},
            {2, -1},
            {3, -1},
            {4, -1}
        };

        // This function will process all containers recursively.
        if (!processContainer(
            version,
            buff,
            jSrc.at("rootContainer"),
            indexMap,
            languages,
            depends,
            defaultLocale
        ))
        {
            fprintf(stderr, "[LANG//DLGE] Failed to process containers!\n");
            return {};
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "DLGE", depends);

        return out;
    }
    catch (const json::exception& err)
    {
        fprintf(stderr, "[LANG//DLGE] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return {};
}
#pragma endregion
