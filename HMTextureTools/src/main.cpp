#include <filesystem>
#include <fstream>
#include <cassert>
#include <iterator>

#include <argparse/argparse.hpp>
#include "EncodingDevice.h"
#include "Global.h"
#include "Texture.h"

argparse::ArgumentParser program("HMTextureTools", "v1.5.2");

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

std::vector<char> readFile(std::string path, bool isTEXD = false)
{
    // Check file exists
    if (!std::filesystem::exists(path))
    {
        LOG((isTEXD ? "The path for the TEXD is invalid. " : "The path for the TEXT/D is invalid. ") << "Please make sure it is correct.");
        LOG_AND_EXIT(program);
    }

    // Load file data into vector
    std::ifstream FILE(path, std::ifstream::binary);
    std::vector<char> fileData((std::istreambuf_iterator<char>(FILE)), std::istreambuf_iterator<char>());
    FILE.close();

    return fileData;
}

int main(int argc, char *argv[])
{
    // Define arguments
    program.add_argument("mode")
        .help("the mode to use: convert or rebuild")
        .required();

    program.add_argument("game")
        .help("the version of the game the texture(s) are from: HMA, H2016, H2, or H3")
        .required();

    program.add_argument("texture")
        .help("path to the texture. convert requires a TEXT/D, rebuild requires a TGA (For H3 pass TEXT path)")
        .required();

    program.add_argument("output_path")
        .help("the path to the output file")
        .required();

    program.add_argument("--texd")
        .help("path to the input TEXD. this is H3 only! does nothing on rebuild or on other games")
        .nargs(1);

    program.add_argument("-p", "--port")
        .help("the game to port to (H2016, H2, or H3. HMA is unsupported). only works on convert, requires TEXT/D (or both depending on game)")
        .nargs(1);

    program.add_argument("--rebuildboth")
        .help("use this option for the input TGA to be downscaled to make a TEXT and TEXD (the .TEXT and .TEXD will be added - only works on rebuild!)")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--texdoutput")
        .help("path to the output TEXD. only for rebuilding both and porting. [currently only works on H3]")
        .nargs(1);

    program.add_argument("--istexd")
        .help("use this option if the input texture is a TEXD (only works on H2016 and H2! rebuilding both overrides this option!)")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--metapath")
        .help("path to the input/output meta")
        .nargs(1);

    program.add_argument("--ps4swizzle")
        .help("use this option if you want deswizzle/swizzle textures (porting with this option will only deswizzle!)")
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

    auto texturePath = program.get<std::string>("texture");
    toLowercase(texturePath);

    auto outPath = program.get<std::string>("output_path");

    bool ps4swizzle = program["--ps4swizzle"] == true;
    bool isTEXD = program["--istexd"] == true;
    bool rebuildBoth = program["--rebuildboth"] == true;

    Texture::Version portTo = Texture::Version::NONE;
    if (program.is_used("--port"))
    {
        auto port = program.get<std::string>("--port");

        toUppercase(port);
        if (port == "HMA" || game == "HMA")
        {
            LOG_AND_EXIT("Porting to/from Hitman: Absolution is not supported!");
        }

        if (game != "H3" && port == "H3")
        {
            LOG_AND_EXIT("You can only port to H3 from H3! Please extract and edit a texture from H3!");
        }

        if (port == "H2016")
        {
            portTo = Texture::Version::H2016;
        }
        else if (port == "H2")
        {
            portTo = Texture::Version::H2;
        }
        else if (port == "H3")
        {
            portTo = Texture::Version::H3;
        }
        else
        {
            LOG("Invalid game specified for port to.");
            LOG_AND_EXIT(program);
        }
    }

    std::string h3TEXDpath = "";
    if (program.is_used("--texd"))
        h3TEXDpath = program.get<std::string>("--texd");

    Texture::Version version;
    if (game == "H2016")
    {
        version = Texture::Version::H2016;
    }
    else if (game == "H2")
    {
        version = Texture::Version::H2;
    }
    else if (game == "H3")
    {
        version = Texture::Version::H3;
    }
    else if (game == "HMA")
    {
        version = Texture::Version::HMA;
    }
    else
    {
        LOG("Invalid game specified.");
        LOG_AND_EXIT(program);
    }

    std::string texdOutputPath = "";
    if (program.is_used("--texdoutput"))
        texdOutputPath = program.get<std::string>("--texdoutput");

    std::string metaPath = "";
    if (program.is_used("--metapath"))
        metaPath = program.get<std::string>("--metapath");

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
        handleHRESULT("Failed to initalise COM!", hr);

    if (mode == "convert")
    {
        if (portTo != Texture::Version::NONE)
            LOG("Porting texture from " + Texture::versionToString(version) + " to " + Texture::versionToString(portTo));
        else
            LOG("Converting " + Texture::versionToString(version) + " texture to TGA...");

        std::vector<char> rawTEXD{};
        if (version == Texture::Version::H3 && program.is_used("--texd"))
            rawTEXD = readFile(h3TEXDpath);

        std::vector<char> rawTEXT = readFile(texturePath);
        switch (version)
        {
        case Texture::Version::HMA:
            Texture::HMA::Convert(rawTEXT, outPath, ps4swizzle, metaPath);
            break;
        case Texture::Version::H2016:
            Texture::H2016::Convert(rawTEXT, outPath, ps4swizzle, portTo, isTEXD, texdOutputPath, metaPath);
            break;
        case Texture::Version::H2:
            Texture::H2::Convert(rawTEXT, outPath, ps4swizzle, portTo, isTEXD, texdOutputPath, metaPath);
            break;
        case Texture::Version::H3:
            Texture::H3::Convert(rawTEXT, rawTEXD, outPath, ps4swizzle, portTo, h3TEXDpath != "", texdOutputPath, metaPath);
            break;
        }
    }
    else if (mode == "rebuild")
    {
        LOG("Rebuilding " + Texture::versionToString(version) + " TGA to TEXT/D...");

        switch (version)
        {
        case Texture::Version::HMA:
            Texture::HMA::Rebuild(texturePath, outPath, ps4swizzle, metaPath);
            break;
        case Texture::Version::H2016:
            Texture::H2016::Rebuild(texturePath, outPath, rebuildBoth, isTEXD, ps4swizzle, rebuildBoth ? texdOutputPath : "", metaPath);
            break;
        case Texture::Version::H2:
            Texture::H2::Rebuild(texturePath, outPath, rebuildBoth, isTEXD, ps4swizzle, rebuildBoth ? texdOutputPath : "", metaPath);
            break;
        case Texture::Version::H3:
            Texture::H3::Rebuild(texturePath, outPath, rebuildBoth, ps4swizzle, rebuildBoth ? texdOutputPath : "", metaPath);
            break;
        }
    }
    else
    {
        LOG("Invalid mode. Must be \"convert\" or \"rebuild\"");
        LOG_AND_EXIT(program);
    }
}
