#include "main.h"

argparse::ArgumentParser program("BOXCExporter", "v0.9.0");

int main(int argc, char *argv[])
{
    // Define arguments
    program.add_argument("mode")
        .help("the mode to use: convert or rebuild")
        .required();

    program.add_argument("input_path")
        .help("convert requires path to a BOXC file. rebuild requires path to a BOXC JSON file.")
        .required();

    program.add_argument("output_path")
        .help("convert requires the path to the output folder (doesn't have to exist). rebuild requires the path to a BOXC file.")
        .required();

    program.add_argument("--noconvert")
        .help("use this option if you don't want to convert the textures to TGA.")
        .default_value(false)
        .implicit_value(true);
    ///////////////////

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        LOG(err.what());
        LOG_AND_EXIT(program);
    }

    bool noconvert = program["--noconvert"] == true;
    std::string mode = program.get<std::string>("mode");
    std::filesystem::path inputPath(program.get<std::string>("input_path"));
    std::filesystem::path outputPath(program.get<std::string>("output_path"));

    if (mode == "convert")
    {
        file_buffer buff;
        buff.load(inputPath.string());

        // Parse the BOXC file
        BOXC boxc{};

        boxc.cubemapCount = buff.read<uint32_t>();

        for (uint32_t i = 0; i < boxc.cubemapCount; i++)
        {
            Cubemap cubemap{};

            cubemap.pos = buff.read<Vector3>();
            cubemap.textureData = buff.read<std::vector<char>>();

            boxc.cubemaps.push_back(cubemap);
        }

        // Setup the output JSON
        json j = {};

        if (!std::filesystem::exists(outputPath))
        {
            std::filesystem::create_directory(outputPath);
        }

        for (uint32_t i = 0; i < boxc.cubemapCount; i++)
        {
            Cubemap cubemap = boxc.cubemaps.at(i);
            json cubemapObj;

            std::string outFilename(inputPath.filename().string() + "_" + std::to_string(i) + (noconvert ? ".bin" : ".tga"));

            if (noconvert)
            {
                std::ofstream outfile(outputPath / outFilename, std::ios::out | std::ios::binary);
                if (!outfile)
                {
                    LOG("Cannot open file to write! Aborting...");
                    LOG(program);
                    return 1;
                }

                outfile.write((const char *)cubemap.textureData.data(), cubemap.textureData.size());
                outfile.close();

                if (!outfile.good())
                {
                    LOG("Error writing output file! Aborting...");
                    LOG(program);
                    return 1;
                }
            }
            else
            {
                DirectX::Blob ddsBuffer{};

                createDDS(cubemap.textureData, ddsBuffer);
                outputToTGA(ddsBuffer, outputPath / outFilename);
            }

            cubemapObj = {
                { "pos", cubemap.pos.toJSON() },
                { "filepath", outFilename }
            };

            j.push_back(cubemapObj);
        }

        std::string outFilename(inputPath.filename().string() + ".json");
        std::ofstream jsonFile(outputPath / outFilename);
        jsonFile << std::setw(4) << j << std::endl;
    }
    else if (mode == "rebuild")
    {
        std::ifstream ifs(inputPath);
        if (!ifs.good())
        {
            LOG("Could not open JSON file for reading!");
            return 1;
        }

        json j;
        ifs >> j;

        if (j.is_null())
        {
            LOG("Failed to parse JSON file!");
            return 1;
        }

        HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
        if (FAILED(hr))
            handleHRESULT("Failed to initalise COM!", hr);

        file_buffer buff;
        buff.write<uint32_t>(j.size());

        std::filesystem::path tgaInput = inputPath.parent_path();
        for (const auto &entry : j)
        {
            buff.write<float>(entry["pos"][0].get<float>());
            buff.write<float>(entry["pos"][1].get<float>());
            buff.write<float>(entry["pos"][2].get<float>());

            buff.write<std::vector<char>>(loadTGA(tgaInput / entry["filepath"].get<std::string>()));
        }

        buff.save(outputPath.generic_string());
    }
    else
    {
        LOG("Invalid mode specified!");
        LOG(program);
        return 1;
    }

    return 0;
}