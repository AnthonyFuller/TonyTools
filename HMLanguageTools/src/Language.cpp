#include "Language.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>

#include <ResourceLib_HM2016.h>
#include <ResourceLib_HM2.h>
#include <ResourceLib_HM3.h>
#include <nlohmann/json.hpp>
#include <hash/md5.h>
#include <hash/crc32.h>

#include "zip.hpp"

using json = nlohmann::json;
using ojson = nlohmann::ordered_json;

// General utility functions
bool is_valid_hash(std::string hash) {
    const std::string valid_chars = "0123456789ABCDEF";

    if (hash.length() != 16) {
        return false;
    }

    std::transform(hash.begin(), hash.end(), hash.begin(), ::toupper);

    for (int i = 0; i < 16; i++) {
        if (valid_chars.find(hash[i]) == std::string::npos) {
            return false;
        }
    }

    return true;
}

std::string computeHash(std::string str)
{
    MD5 md5;
    str = md5(str);
    str = "00" + str.substr(2, 14);

    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    return str;
}

std::string generateMeta(std::string hash, uint32_t size, std::string type, std::vector<std::pair<std::string, std::string>> depends)
{
    json j = {
        { "hash_value", is_valid_hash(hash) ? hash : computeHash(hash) },
        { "hash_offset", 0x10000000 },
        { "hash_size", 0x80000000 + size },
        { "hash_resource_type", type },
        { "hash_reference_table_size", (0x9 * depends.size()) + 4 },
        { "hash_reference_table_dummy", 0 },
        { "hash_size_final", size },
        { "hash_size_in_memory", 0xFFFFFFFF },
        { "hash_size_in_video_memory", 0xFFFFFFFF },
        { "hash_reference_data", json::array() }
    };

    for (const auto& [hash, flag] : depends) {
        j.at("hash_reference_data").push_back({
            { "hash", hash },
            { "flag", flag }
        });
    }

    return j.dump();
}

ResourceConverter* getConverter(Language::Version version, const char* resourceType)
{
    switch (version) {
        case Language::Version::H2016: {
            if (!HM2016_IsResourceTypeSupported(resourceType)) {
                fprintf(stderr, "[LANG] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
                return nullptr;
            }

            return HM2016_GetConverterForResource(resourceType);
        };
        case Language::Version::H2: {
            if (!HM2_IsResourceTypeSupported(resourceType)) {
                fprintf(stderr, "[LANG] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
                return nullptr;
            }

            return HM2_GetConverterForResource(resourceType);
        };
        case Language::Version::H3: {
            if (!HM3_IsResourceTypeSupported(resourceType)) {
                fprintf(stderr, "[LANG] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
                return nullptr;
            }

            return HM3_GetConverterForResource(resourceType);
        };
        default:
            return nullptr;
    }
}

ResourceGenerator* getGenerator(Language::Version version, const char* resourceType)
{
    switch (version) {
        case Language::Version::H2016: {
            if (!HM2016_IsResourceTypeSupported(resourceType)) {
                fprintf(stderr, "[LANG] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
                return nullptr;
            }

            return HM2016_GetGeneratorForResource(resourceType);
        };
        case Language::Version::H2: {
            if (!HM2_IsResourceTypeSupported(resourceType)) {
                fprintf(stderr, "[LANG] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
                return nullptr;
            }

            return HM2_GetGeneratorForResource(resourceType);
        };
        case Language::Version::H3: {
            if (!HM3_IsResourceTypeSupported(resourceType)) {
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
    std::regex regex{ R"([,]+)" };
    std::sregex_token_iterator it{ str.begin(), str.end(), regex, -1 };
    return std::vector<std::string>{ it, {} };
}

// From https://github.com/glacier-modding/RPKG-Tool/blob/145d8d7d9711d57f1434489706c3d81b2feeed73/src/crypto.cpp#L3-L41
constexpr uint32_t xteaKeys[4] = { 0x53527737, 0x7506499E, 0xBD39AEE3, 0xA59E7268 };
constexpr uint32_t xteaDelta = 0x9E3779B9;
constexpr uint32_t xteaRounds = 32;

std::string xteaDecrypt(std::vector<char> data)
{
    for (uint32_t i = 0; i < data.size() / 8; i++) {
        uint32_t* strV0 = (uint32_t*)(data.data() + (i * 8));
        uint32_t* strV1 = (uint32_t*)(data.data() + (i * 8) + 4);

        uint32_t v0 = *strV0;
        uint32_t v1 = *strV1;
        uint32_t sum = xteaDelta * xteaRounds;

        for (uint32_t i = 0; i < xteaRounds; i++) {
            v1 -= (v0 << 4 ^ v0 >> 5) + v0 ^ sum + xteaKeys[sum >> 11 & 3];
            sum -= xteaDelta;
            v0 -= (v1 << 4 ^ v1 >> 5) + v1 ^ sum + xteaKeys[sum & 3];
        }

        *strV0 = v0;
        *strV1 = v1;
    }

    return std::string(data.begin(), std::find(data.begin(), data.end(), '\0'));
}

std::vector<char> xteaEncrypt(std::string str)
{
    std::vector<char> data(str.begin(), str.end());
    uint32_t paddedSize = data.size() + (data.size() % 8 == 0 ? 0 : 8 - (data.size() % 8));
    data.resize(paddedSize, '\0');

    for (uint32_t i = 0; i < paddedSize / 8; i++) {
        uint32_t* vecV0 = (uint32_t*)(data.data() + (i * 8));
        uint32_t* vecV1 = (uint32_t*)(data.data() + (i * 8) + 4);

        uint32_t v0 = *vecV0;
        uint32_t v1 = *vecV1;
        uint32_t sum = 0;

        for (uint32_t i = 0; i < xteaRounds; i++) {
            v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + xteaKeys[sum & 3]);
            sum += xteaDelta;
            v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + xteaKeys[(sum >> 11) & 3]);
        }

        *vecV0 = v0;
        *vecV1 = v1;
    }

    return data;
}

uint32_t getWavHashFromPath(std::string path)
{
    if (is_valid_hash(path)) return -1;

    std::regex r{ R"([^\/]*(?=\.wav))" };
    std::smatch m;
    std::regex_search(path, m, r);

    if (m.size() != 1) return -1;

    CRC32 crc32;

    return std::strtoul(crc32(m[0]).c_str(), nullptr, 16);
}

// RTLV
std::string Language::RTLV::Convert(Language::Version version, std::vector<char> data, std::string metaJson)
{
    ResourceConverter* converter = getConverter(version, "RTLV");
    if (!converter) return "";

    JsonString* converted = converter->FromMemoryToJsonString(data.data(), data.size());
    if (!converted) return "";

    ojson j = {
        { "hash", "" },
        { "videos", json::object()},
        { "subtitles", json::object() }
    };

    try {
        ojson jConv = json::parse(converted->JsonData);
        converter->FreeJsonString(converted);
        converted = nullptr;

        if (jConv.at("AudioLanguages").size() != jConv.at("VideoRidsPerAudioLanguage").size()) {
            fprintf(stderr, "[LANG//RTLV] Mismatch in languages and resource IDs in RL JSON!\n");
            return "";
        }

        for (const auto& [lang, id] : c9::zip(jConv.at("AudioLanguages"), jConv.at("VideoRidsPerAudioLanguage"))) {
            j["videos"][lang] = std::format("{:08X}{:08X}",
                id.at("m_IDHigh").get<uint32_t>(), id.at("m_IDLow").get<uint32_t>()
            );
        }

        if (jConv.at("SubtitleLanguages").size() != jConv.at("SubtitleMarkupsPerLanguage").size()) {
            fprintf(stderr, "[LANG//RTLV] Mismatch in subtitle languages and content in RL JSON!\n");
            return "";
        }

        for (const auto& [lang, text] : c9::zip(jConv.at("SubtitleLanguages"), jConv.at("SubtitleMarkupsPerLanguage"))) {
            j["subtitles"][lang] = text;
        }

        ojson meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    } catch(json::exception err) {
        if (converted) converter->FreeJsonString(converted);

        fprintf(stderr, "[LANG//RTLV] JSON error:\n");
        fprintf(stderr, "\t%s\n", err.what());
        return "";
    }
}

Language::Rebuilt Language::RTLV::Rebuild(Language::Version version, std::string jsonString)
{
    ResourceGenerator* generator = getGenerator(version, "RTLV");
    if (!generator) return {};

    Language::Rebuilt out{};
    std::vector<std::pair<std::string, std::string>> depends{};

    try {
        ojson jSrc = json::parse(jsonString);

        if (jSrc.at("videos").size() < 1 || jSrc.at("subtitles").size() < 1) {
            fprintf(stderr, "[LANG//RTLV] Videos and/or subtitles object is empty!\n");
            return {};
        }

        ojson j = {
            { "AudioLanguages", {} },
            { "VideoRidsPerAudioLanguage", {} },
            { "SubtitleLanguages", {} },
            { "SubtitleMarkupsPerLanguage", {} }
        };

        for (auto& [lang, video] : jSrc.at("videos").items()) {
            j.at("AudioLanguages").push_back(lang);

            video = video.get<std::string>();
            if (!is_valid_hash(video)) video = computeHash(video);
        
            uint64_t id = std::strtoull(video.get<std::string>().c_str(), nullptr, 16);
            if (id == 0 || id == ULLONG_MAX) {
                fprintf(stderr, "[LANG//RTLV] Could not convert hash for video[\"%s\"] to integer!", lang.c_str());
                return {};
            }

            j.at("VideoRidsPerAudioLanguage").push_back(json::object({
                { "m_IDHigh", id >> 32 },
                { "m_IDLow", id & 0xFFFFFFFF }
            }));

            depends.push_back(std::make_pair<std::string, std::string>(
                video,
                depends.size() >= 1 ? "8B" : "81"
            ));
        }

        for (auto& [lang, text] : jSrc.at("subtitles").items()) {
            j.at("SubtitleLanguages").push_back(lang);
            j.at("SubtitleMarkupsPerLanguage").push_back(text);
        }

        std::string rlJson = j.dump();
        ResourceMem* generated = generator->FromJsonStringToResourceMem(rlJson.c_str(), rlJson.size(), false);
        if (!generated) return {};

        out.file.resize(generated->DataSize);
        std::memcpy(out.file.data(), generated->ResourceData, generated->DataSize);
        generator->FreeResourceMem(generated);

        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "RTLV", depends);

        return out;
    } catch(json::exception err) {
        fprintf(stderr, "[LANG//RTLV] JSON error:\n");
        fprintf(stderr, "\t%s\n", err.what());
        return {};
    }
}

// LOCR
std::string Language::LOCR::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap)
{
    buffer buff(data);

    bool isLOCRv2 = false;
    if (version != Version::H2016) {
        buff.read<uint8_t>();
        isLOCRv2 = true;
    }

    ojson j = {
        { "hash", "" },
        { "languages", json::object() }
    };

    uint32_t numLanguages = (buff.read<uint32_t>() - isLOCRv2) / 4;
    buff.index -= 4;
    std::vector<std::string> languages;
    if (!langMap.empty()) {
        languages = split(langMap);
    } else if (version == Version::H3) {
        languages = { "xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp" };
    } else {
        languages = { "xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc" };
    }

    if (numLanguages > languages.size()) {
        fprintf(stderr, "[LANG//LOCR] Language map is smaller than the number of languages in the file!\n");
        return "";
    }

    size_t oldIndex = buff.index;
    for (int i = 0; i < numLanguages; i++) {
        j.at("languages").push_back({ languages.at(i), json::object() });

        uint32_t oldOffset = buff.index;
        uint32_t offset;
        buff.index = oldIndex;
        if ((offset = buff.read<uint32_t>()) == 0xFFFFFFFF) {
            buff.index = oldOffset;
            oldIndex += 4;
            continue;
        }
        buff.index = offset;
        oldIndex += 4;

        uint32_t numStrings = buff.read<uint32_t>();
        for (int k = 0; k < numStrings; k++) {
            uint32_t hash = buff.read<uint32_t>();
            std::string str = xteaDecrypt(buff.read<std::vector<char>>());
            buff.index += 1;

            j.at("languages").at(languages.at(i)).push_back({ std::format("{:X}", hash), str });
        }
    }

    if (buff.index != buff.size()) {
        fprintf(stderr, "[LANG//LOCR] Did not read to end of file! Report this to the author!\n");
    }

    try {
        ojson meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    } catch(json::exception err) {
        fprintf(stderr, "[LANG//LOCR] JSON error:\n");
        fprintf(stderr, "\t%s\n", err.what());
        return "";
    }
}

Language::Rebuilt Language::LOCR::Rebuild(Language::Version version, std::string jsonString)
{
    Language::Rebuilt out{};
    std::vector<std::pair<std::string, std::string>> depends{};
    
    try {
        ojson jSrc = nlohmann::ordered_json::parse(jsonString);

        buffer buff;

        if (version != Version::H2016) buff.write<char>('\0');

        uint32_t curOffset = buff.index;
        buff.insert(jSrc.at("languages").size() * 4);

        CRC32 crc32;

        for (auto& [lang, strings] : jSrc.at("languages").items()) {
            if (strings.size() == 0) {
                uint32_t temp = buff.index;
                buff.index = curOffset;
                buff.write<uint32_t>(0xFFFFFFFF);
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
            for (auto& [strHash, string] : jSrc.at("languages").at(lang).items()) {
                uint32_t hash = std::strtoul(strHash.c_str(), nullptr, 16);
                if (hash == 0 || hash == ULLONG_MAX) {
                    hash = std::strtoul(crc32(strHash).c_str(), nullptr, 16);
                }

                buff.write<uint32_t>(hash);
                buff.write<std::vector<char>>(xteaEncrypt(string.get<std::string>()));
                buff.write<char>('\0');
            }
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "LOCR", depends);

        return out;
    } catch(json::exception err) {
        fprintf(stderr, "[LANG//LOCR] JSON error:\n");
        fprintf(stderr, "\t%s\n", err.what());
        return {};
    }
}

// DLGE
std::string Language::DLGE::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap)
{
    buffer buff(data);

    ojson j = {
        { "hash", "" },
        { "DITL", "" },
        { "CLNG", "" },
        { "wavs", json::object() },
        { "containers", json::array() }
    };

    // These are (late) 2016/H2 languages by default
    std::vector<std::string> languages = { "xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc" };
    if (!langMap.empty()) {
        languages = split(langMap);
    } else if (version == Version::H3) {
        languages = { "xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp" };
    }

    try {
        ojson meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");
        j["DITL"] = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");
        j["CLNG"] = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");

        uint32_t numOfSections = 0;

        while (buff.index != (buff.size() - 2)) {
            switch (buff.peek<uint8_t>()) {
                case 0x01: { // eDEIT_WavFile
                    buff.index += 1; // we don't need to record the type
                    uint32_t soundTagHash = buff.read<uint32_t>();
                    uint32_t wavNameHash = buff.read<uint32_t>();
                    assert(buff.read<uint32_t>() == 0);

                    std::string wavName = "";
                    std::string soundTagName = "";

                    if (TagMap.contains(soundTagHash)) {
                        soundTagName = TagMap.at(soundTagHash);
                    } else {
                        soundTagName = std::format("{:X}", soundTagHash);
                    }

                    for (uint32_t i = 0; i < languages.size(); i++) {
                        if (version == Version::H2016) {
                            assert(buff.read<uint32_t>() == 0);
                        }
                        
                        uint32_t wavIndex = buff.read<uint32_t>(); // WWES/WWEM depend index
                        uint32_t ffxIndex = buff.read<uint32_t>(); // FaceFX depend index

                        uint32_t subtitleSize = buff.read<uint32_t>();
                        if (subtitleSize != 0) {

                        }
                    }
                    break;
                }
                // Right now, we only have basic information on these containers,
                // so we will read them the same and output them as "known" JSON
                // structs so we can then do more reverse engineering
                case 0x02: // eDEIT_RandomContainer
                case 0x03: // eDEIT_SwitchContainer
                case 0x04: { // eDEIT_SequenceContainer
                    Container container(buff);
                    break;
                }
                case 0x15: { // eDEIT_Invalid
                    fprintf(stderr, "[LANG//DLGE] Invalid section found. Report this!\n");
                    return "";
                }
                default: { // Just in case
                    fprintf(stderr, "[LANG//DLGE] Unknown section found [0x%02X]. Report this!\n", buff.read<uint8_t>());
                    return "";
                }
            }
        }

        return j.dump();
    } catch(json::exception err) {
        fprintf(stderr, "[LANG//DLGE] JSON error:\n");
        fprintf(stderr, "\t%s\n", err.what());
        return "";
    }
}

Language::Rebuilt Language::DLGE::Rebuild(Language::Version version, std::string jsonString)
{
    return {};
}