#include <filesystem>
#include <fstream>
#include <iostream>
#include <cassert>
#include <iterator>

#include <argparse/argparse.hpp>
#include <TonyTools/Languages.h>

using namespace TonyTools;

#define LOG(x) std::cout << x << std::endl
#define LOG_AND_EXIT(x) std::cout << x << std::endl; std::exit(0)

argparse::ArgumentParser program("HMLanguageTools", "v1.6.0");

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

std::vector<char> readFile(std::string path, bool isMeta = false)
{
    // Check file exists
    if (!std::filesystem::exists(path))
    {
        LOG((isMeta ? "The path for the meta is invalid. " : "The path for the input file is invalid. ") << "Please make sure it is correct.");
        LOG_AND_EXIT(program);
    }

    // Load file data into vector
    std::ifstream FILE(path, std::ifstream::binary);
    std::vector<char> fileData((std::istreambuf_iterator<char>(FILE)), std::istreambuf_iterator<char>());
    FILE.close();

    return fileData;
}

void writeFile(std::string path, const char* ptr, size_t size)
{
    if (std::filesystem::exists(path))
    {
        LOG("File exists at one of the output locations, overriding!");
    }

    std::ofstream FILE(path, std::ios_base::binary);
    if (!FILE.good())
    {
        LOG_AND_EXIT("Could not open file for writing!");
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

    program.add_argument("type")
        .help("the type of file: CLNG, DITL, DLGE, LOCR, or RLTV")
        .required();

    program.add_argument("input_path")
        .help("path to the input file")
        .required();

    program.add_argument("output_path")
        .help("the path to the output file")
        .required();

    program.add_argument("--metapath")
        .help("path to the input/output for the meta JSON (RPKG Tool!)")
        .nargs(1);

    program.add_argument("--langmap")
        .help("custom language map, overrides the one provided by version e.g. xx,en,tc,am,on,gu,ss")
        .nargs(1);

    program.add_argument("--defaultlocale")
        .help("the default audio locale, used for DLGE conversion")
        .default_value(std::string("en"))
        .nargs(1);

    program.add_argument("--hexprecision")
        .help("should random weights be output as their hex variants, used for DLGE convert only")
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

    auto mode = program.get<std::string>("mode");
    toLowercase(mode);

    auto game = program.get<std::string>("game");
    toUppercase(game);

    auto type = program.get<std::string>("type");
    toUppercase(type);

    auto inputPath = program.get<std::string>("input_path");
    toLowercase(inputPath);

    auto outPath = program.get<std::string>("output_path");

    auto defLocale = program.get<std::string>("--defaultlocale");

    auto hexPrecision = program.get<bool>("--hexprecision");

    Language::Version version;
    if (game == "H2016")
    {
        version = Language::Version::H2016;
    }
    else if (game == "H2")
    {
        version = Language::Version::H2;
    }
    else if (game == "H3")
    {
        version = Language::Version::H3;
    }
    else
    {
        LOG("Invalid game specified.");
        return 1;
    }

    std::string metaPath = inputPath + ".meta.json ";
    if (program.is_used("--metapath"))
        metaPath = program.get<std::string>("--metapath");
    else
        LOG("Meta path not specified. Defaulting to input + .meta.json!");

    if (mode == "convert")
    {
        if (!std::filesystem::exists(metaPath))
        {
            LOG("Meta could not be found! Please specify it with --metapath!");
            return 1;
        }

        std::vector<char> metaFileData = readFile(metaPath, true);
        std::string output = "";

        if (type == "CLNG")
        {
            output = Language::CLNG::Convert(version, readFile(inputPath), std::string(metaFileData.begin(), metaFileData.end()),
                program.is_used("--langmap") ? program.get<std::string>("--langmap") : ""
            );
        }
        else if (type == "DITL")
        {
            output = Language::DITL::Convert(readFile(inputPath), std::string(metaFileData.begin(), metaFileData.end()));
        }
        else if (type == "DLGE")
        {
            output = Language::DLGE::Convert(version, readFile(inputPath), std::string(metaFileData.begin(), metaFileData.end()),
                defLocale, hexPrecision, program.is_used("--langmap") ? program.get<std::string>("--langmap") : ""
            );
        }
        else if (type == "LOCR")
        {
            output = Language::LOCR::Convert(version, readFile(inputPath), std::string(metaFileData.begin(), metaFileData.end()),
                program.is_used("--langmap") ? program.get<std::string>("--langmap") : ""
            );
        }
        else if (type == "RTLV")
        {
            output = Language::RTLV::Convert(version, readFile(inputPath), std::string(metaFileData.begin(), metaFileData.end()));
        }
        else
        {
            LOG(type);
            LOG("Invalid type specified.");
            return 1;
        }

        if (output.empty()) {
            LOG("Failed to convert " << type << " to JSON!");
            return 1;
        }

        writeFile(outPath, output.data(), output.size());

        LOG("Successfully converted " << type << " to JSON!");
    }
    else if (mode == "rebuild")
    {
        std::vector<char> inputFileData = readFile(inputPath);
        Language::Rebuilt output{};

        if (type == "CLNG")
        {
            output = Language::CLNG::Rebuild(std::string(inputFileData.begin(), inputFileData.end()));
        }
        else if (type == "DITL")
        {
            output = Language::DITL::Rebuild(std::string(inputFileData.begin(), inputFileData.end()));
        }
        else if (type == "DLGE")
        {
            output = Language::DLGE::Rebuild(version, std::string(inputFileData.begin(), inputFileData.end()), defLocale,
                program.is_used("--langmap") ? program.get<std::string>("--langmap") : ""
            );
        }
        else if (type == "LOCR")
        {
            output = Language::LOCR::Rebuild(version, std::string(inputFileData.begin(), inputFileData.end()));
        }
        else if (type == "RTLV")
        {
            output = Language::RTLV::Rebuild(version, std::string(inputFileData.begin(), inputFileData.end()),
                program.is_used("--langmap") ? program.get<std::string>("--langmap") : ""
            );
        }
        else
        {
            LOG("Invalid type specified.");
            return 1;
        }

        if (output.file.empty() || output.meta.empty()) {
            LOG("Failed to convert JSON to " << type << "!");
            return 1;
        }

        writeFile(outPath, output.file.data(), output.file.size());
        writeFile(metaPath, output.meta.data(), output.meta.size());

        LOG("Successfully converted JSON to " << type << " + meta!");
    }
    else
    {
        LOG("Invalid mode. Must be \"convert\" or \"rebuild\"");
        return 1;
    }

    return 0;
}
