#include "main.h"

argparse::ArgumentParser program("BOXCExporter", "v0.9.0");

int main(int argc, char *argv[])
{
    // Define arguments
    program.add_argument("mode")
        .help("the mode to use: convert")
        .required();

    program.add_argument("boxc")
        .help("path to the BOXC file.")
        .required();

    program.add_argument("output_path")
        .help("the path to the output folder (doesn't have to exist)")
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

    std::filesystem::path inputPath(program.get<std::string>("boxc"));
    file_buffer buff;
    buff.load(inputPath.string());

    // Parse the BOXC file
    BOXC boxc{};

    boxc.cubemapCount = buff.read<uint32_t>();

    for (int i = 0; i < boxc.cubemapCount; i++)
    {
        Cubemap cubemap{};

        cubemap.pos = buff.read<Vector3>();
        cubemap.textureData = buff.read<std::vector<char>>();

        boxc.cubemaps.push_back(cubemap);
    }

    // Setup the output JSON
    rapidjson::Document json;
    rapidjson::Document::AllocatorType &allocator = json.GetAllocator();
    json.SetArray();

    std::filesystem::path outputPath(program.get<std::string>("output_path"));
    if (!std::filesystem::exists(outputPath))
    {
        std::filesystem::create_directory(outputPath);
    }

    bool noconvert = program["--noconvert"] == true;
    std::string mode = program.get<std::string>("mode");

    if (mode == "convert")
    {
        for (int i = 0; i < boxc.cubemapCount; i++)
        {
            Cubemap cubemap = boxc.cubemaps.at(i);
            rapidjson::Value cubemapObj(rapidjson::kObjectType);

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

            cubemapObj.AddMember("pos", cubemap.pos.toJSON(allocator), allocator);
            cubemapObj.AddMember("filename", outFilename, allocator);

            json.PushBack(cubemapObj, allocator);
        }

        rapidjson::StringBuffer json_buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> json_writer(json_buffer);
        json.Accept(json_writer);

        std::string outFilename(inputPath.filename().string() + ".json");
        std::ofstream jsonFile(outputPath / outFilename);
        jsonFile.write(json_buffer.GetString(), json_buffer.GetSize());
        jsonFile.close();
    }
    else
    {
        LOG("Invalid mode specified!");
        LOG(program);
        return 1;
    }

    return 0;
}