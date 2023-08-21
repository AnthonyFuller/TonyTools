#include "GFXFzip.h"

#include <map>
#include <regex>

#include <ResourceLib_HM2016.h>
#include <ResourceLib_HM2.h>
#include <ResourceLib_HM3.h>
#include <bit7z/bitarchivewriter.hpp>
#include <bit7z/bitarchivereader.hpp>
#include <nlohmann/json.hpp>

#include "Base64.h"

using json = nlohmann::ordered_json;

std::string generateMeta(std::string hash, uint32_t size)
{
    json j = {
        {"hash_value", hash},
        {"hash_offset", 0x10000000},
        {"hash_size", 0x80000000 + size},
        {"hash_resource_type", "GFXF"},
        {"hash_reference_table_size", 0},
        {"hash_reference_table_dummy", 0},
        {"hash_size_final", size},
        {"hash_size_in_memory", ULONG_MAX},
        {"hash_size_in_video_memory", ULONG_MAX},
        {"hash_reference_data", json::array()}
    };

    return j.dump();
}

ResourceConverter* getConverter(GFXFzip::Version version, const char *resourceType)
{
    switch (version)
    {
    case GFXFzip::Version::H2016:
    {
        if (!HM2016_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[GFXFzip] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2016_GetConverterForResource(resourceType);
    };
    case GFXFzip::Version::H2:
    {
        if (!HM2_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[GFXFzip] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2_GetConverterForResource(resourceType);
    };
    case GFXFzip::Version::H3:
    {
        if (!HM3_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[GFXFzip] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM3_GetConverterForResource(resourceType);
    };
    default:
        return nullptr;
    }
}

ResourceGenerator* getGenerator(GFXFzip::Version version, const char *resourceType)
{
    switch (version)
    {
    case GFXFzip::Version::H2016:
    {
        if (!HM2016_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[GFXFzip] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2016_GetGeneratorForResource(resourceType);
    };
    case GFXFzip::Version::H2:
    {
        if (!HM2_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[GFXFzip] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2_GetGeneratorForResource(resourceType);
    };
    case GFXFzip::Version::H3:
    {
        if (!HM3_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[GFXFzip] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM3_GetGeneratorForResource(resourceType);
    };
    default:
        return nullptr;
    }
}

// GFXF -> Zip
std::vector<uint8_t> GFXFzip::Convert(Version version, std::vector<uint8_t> data, std::vector<uint8_t> metaFileData)
{
    ResourceConverter *converter = getConverter(version, "GFXF");
    if (!converter)
        return {};

    JsonString *converted = converter->FromMemoryToJsonString(data.data(), data.size());
    if (!converted)
        return {};

    json j;
    json jMeta;
    json meta = json::object();
    std::string swfName = "";
    try {
        j = json::parse(converted->JsonData);
        jMeta = json::parse(std::string(metaFileData.begin(), metaFileData.end()));

        if (jMeta["hash_path"].is_null())
        {
        failedName:
            fprintf(stderr, "[GFXFzip] Unable to find name of swf, defaulting to hash.\n");
            swfName = jMeta.at("hash_value");
        }
        else
        {
            std::string hashPath = jMeta.at("hash_path").get<std::string>();
            std::regex r{R"([^\/]*(?=\.swf))"};
            std::smatch m;
            std::regex_search(hashPath, m, r);

            if (m.size() != 1)
                goto failedName;
            else
                swfName = m[0];
        }

        swfName += ".gfx";

        meta.push_back({ "hash", jMeta.at("hash_value") });
    } catch (const json::exception& err) {
        if (converted)
            converter->FreeJsonString(converted);

        fprintf(stderr, "[GFXFzip] JSON error:\n"
                        "\t%s\n", err.what());

        return {};
    }

    if (j.at("m_pAdditionalFileNames").size() != j.at("m_pAdditionalFileData").size())
    {
        fprintf(stderr, "[GFXFzip] Mismatched additional files!\n");
        return {};
    }

    try {
        using namespace bit7z;

        Bit7zLibrary lib{ "7z.dll" };
        BitArchiveWriter arc{ lib, BitFormat::Zip };

        // Due to the way bit7z works, we need to keep the vector somewhere in memory before compressing.
        std::map<std::string, std::vector<uint8_t>> files = {};

        std::string encodedSwf = j.at("m_pSwfData").get<std::string>();
        std::string decodedSwf;
        Base64::Decode(encodedSwf, decodedSwf);
        files[swfName] = std::vector<uint8_t>(decodedSwf.begin(), decodedSwf.end());

        std::string metaString = meta.dump();
        files["meta.json"] = std::vector<uint8_t>(metaString.begin(), metaString.end());

        uint32_t fileCount = j.at("m_pAdditionalFileNames").size();
        for (uint32_t i = 0; i < fileCount; i++)
        {
            std::string name = j.at("m_pAdditionalFileNames").at(i).get<std::string>();
            std::string encodedData = j.at("m_pAdditionalFileData").at(i).get<std::string>();

            std::string decodedData;
            Base64::Decode(encodedData, decodedData);
            
            files[name] = std::vector<uint8_t>(decodedData.begin(), decodedData.end());
        }

        for (const auto &[name, data] : files)
            arc.addFile(data, name);

        std::vector<uint8_t> out;
        arc.compressTo(out);

        return out;
    } catch (const bit7z::BitException& err) {
        if (converted)
            converter->FreeJsonString(converted);

        fprintf(stderr, "[GFXFzip] Zip error:\n"
                        "\t%s\n", err.what());
    }

    return {};
}

GFXFzip::Rebuilt GFXFzip::Rebuild(GFXFzip::Version version, std::vector<uint8_t> data)
{
    ResourceGenerator *generator = getGenerator(version, "GFXF");
    if (!generator)
        return {};

    Rebuilt out{};

    try {
        using namespace bit7z;

        Bit7zLibrary lib{ "7z.dll" };
        BitArchiveReader arc{ lib, data, BitFormat::Zip };

        std::map<std::string, std::vector<uint8_t>> files;
        arc.extract(files);

        if (!files.contains("meta.json"))
        {
            fprintf(stderr, "[GFXFzip] Could not find meta.json within archive, cannot continue!\n");
            return {};
        }

        json meta = json::parse(std::string(files.at("meta.json").begin(), files.at("meta.json").end()));
        files.erase("meta.json");
        out.hash = meta.at("hash").get<std::string>();

        json j = {
            {"m_pSwfData", nullptr},
            {"m_pAdditionalFileNames", json::array()},
            {"m_pAdditionalFileData", json::array()},
        };
        bool swfFound = false;

        for (const auto &[name, data] : files)
        {
            // Special case for handling swf files.
            if (name.ends_with(".gfx"))
            {
                if (swfFound)
                {
                    fprintf(stderr, "[GFXFzip] Multiple gfx files found within archive, cannot continue!\n");
                    return {};
                }

                j.at("m_pSwfData") = Base64::Encode(std::string(data.begin(), data.end()));

                swfFound = true;
                continue;
            }

            // This file is included in the "additonal files" section, usually dds, but we will include disregarding the extension.
            j.at("m_pAdditionalFileNames").push_back(name);
            j.at("m_pAdditionalFileData").push_back(Base64::Encode(std::string(data.begin(), data.end())));
        }

        if (!swfFound)
        {
            fprintf(stderr, "[GFXFzip] Could not find gfx file found within archive, cannot continue!\n");
            return {};
        }

        std::string rlJson = j.dump();
        ResourceMem *generated = generator->FromJsonStringToResourceMem(rlJson.c_str(), rlJson.size(), false);
        if (!generated)
        {
            fprintf(stderr, "[GFXFzip] Failed to convert ResourceLib JSON to GFXF, cannot continue!\n");
            return {};
        }

        out.file.resize(generated->DataSize);
        std::memcpy(out.file.data(), generated->ResourceData, generated->DataSize);
        generator->FreeResourceMem(generated);

        out.meta = generateMeta(out.hash, out.file.size());

        return out;
    } catch (const bit7z::BitException& err) {
        fprintf(stderr, "[GFXFzip] Zip error:\n"
                        "\t%s\n", err.what());
    } catch (const json::exception& err) {
        fprintf(stderr, "[GFXFzip] JSON error:\n"
                        "\t%s\n", err.what());
    }

    return {};
}
