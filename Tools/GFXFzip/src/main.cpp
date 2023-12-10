#include <iostream>
#include <filesystem>
#include <fstream>

#include "GFXFzip.h"

#include <argparse/argparse.hpp>

#define LOG(x) std::cout << x << std::endl
#define LOG_AND_EXIT(x, code) std::cout << x << std::endl; std::exit(code)

namespace fs = std::filesystem;

argparse::ArgumentParser program("GFXFzip", "v1.8.2");

void toUppercase(std::string &inputstr)
{
    std::transform(inputstr.begin(), inputstr.end(), inputstr.begin(), [](unsigned char c)
                   { return std::toupper(c); });
}

void toLowercase(std::string &inputstr)
{
    std::transform(inputstr.begin(), inputstr.end(), inputstr.begin(), [](unsigned char c)
                   { return std::tolower(c); });
}

std::vector<uint8_t> readFile(std::string path, bool isMeta = false)
{
    // Check file exists
    if (!fs::exists(path))
    {
        LOG((isMeta ? "The path for the meta is invalid. " : "The path for the input file is invalid. ") << "Please make sure it is correct.");
        LOG_AND_EXIT(program, 1);
    }

    // Load file data into vector
    std::basic_ifstream<uint8_t> FILE(path, std::ifstream::binary);
    std::vector<uint8_t> fileData((std::istreambuf_iterator<uint8_t>(FILE)), std::istreambuf_iterator<uint8_t>());
    FILE.close();

    return fileData;
}

void writeFile(std::string path, const uint8_t* ptr, size_t size)
{
    if (fs::exists(path))
    {
        LOG("File exists at one of the output locations, overriding!");
    }

    std::basic_ofstream<uint8_t> FILE(path, std::ios_base::binary);
    if (!FILE.good())
    {
        LOG_AND_EXIT("Could not open file for writing!", 1);
        return;
    }

    FILE.write(ptr, size);
}

int main(int argc, char *argv[])
{
    // Define arguments
    program.add_argument("mode")
        .help("the mode to use: convert or rebuild")
        .required();

    program.add_argument("game")
        .help("the game the file is from: H2016, H2, or H3")
        .required();

    program.add_argument("input_path")
        .help("path to the input file")
        .required();

    program.add_argument("output_path")
        .help("the path to the output file (or folder if using --folder)")
        .required();

    program.add_argument("--metapath")
        .help("path to the input/output for the meta JSON (RPKG Tool!)")
        .nargs(1);

    program.add_argument("--folder")
        .help("if the output_path is set to a folder. should be used if the output hash is unknown")
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
        LOG_AND_EXIT(program, 1);
    }

    auto mode = program.get<std::string>("mode");
    toLowercase(mode);

    auto game = program.get<std::string>("game");
    toUppercase(game);

    auto inputPath = program.get<std::string>("input_path");

    auto outPath = program.get<std::string>("output_path");

    GFXFzip::Version version;
    if (game == "H2016")
    {
        version = GFXFzip::Version::H2016;
    }
    else if (game == "H2")
    {
        version = GFXFzip::Version::H2;
    }
    else if (game == "H3")
    {
        version = GFXFzip::Version::H3;
    }
    else
    {
        LOG("Invalid game specified.");
        return 1;
    }

    if (program.get<bool>("--folder") && !fs::is_directory(fs::path(outPath)))
    {
        LOG_AND_EXIT("Output path is not a folder but the folder option has been specified. Cannot continue.", 1);
    }

    std::string metaPath = (mode == "convert" ? inputPath : outPath) + ".meta.json";
    if (program.is_used("--metapath"))
        metaPath = program.get<std::string>("--metapath");
    else
        LOG("Meta path not specified. Defaulting to " << (mode == "convert" ? "input" : "output") << " + .meta.json!");

    if (mode == "convert")
    {
        if (program.get<bool>("--folder"))
        {
            fs::path inPath(inputPath);
            fs::path outputPath(outPath);
            
            outPath = (outputPath / inPath.filename().concat(".zip")).generic_string();
        }

        std::vector<uint8_t> output = GFXFzip::Convert(version, readFile(inputPath), readFile(metaPath, true));

        if (output.empty()) {
            LOG("Failed to convert GFXF to zip!");
            return 1;
        }

        writeFile(outPath, output.data(), output.size());

        LOG("Successfully converted GFXF to zip!");
    }
    else if (mode == "rebuild")
    {        
        GFXFzip::Rebuilt output = GFXFzip::Rebuild(version, readFile(inputPath));

        if (output.file.empty() || output.meta.empty()) {
            LOG("Failed to convert zip to GFXF!");
            return 1;
        }

        if (program.get<bool>("--folder"))
        {
            fs::path inPath(inputPath);
            fs::path outputPath(outPath);
            
            outPath = (outputPath / (output.hash + ".GFXF")).generic_string();
            metaPath = outPath + ".meta.json";
        }

        writeFile(outPath, output.file.data(), output.file.size());
        writeFile(metaPath, (const uint8_t*)output.meta.data(), output.meta.size());

        LOG("Successfully converted zip to GFXF + meta.json!");
    }
    else
    {
        LOG("Invalid mode. Must be \"convert\" or \"rebuild\"");
        return 1;
    }

    return 0;
}
