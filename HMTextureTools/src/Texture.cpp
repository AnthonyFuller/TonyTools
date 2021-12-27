#include "Texture.h"

void assert_msg(bool expression, std::string string)
{
    if (!(expression))
    {
        LOG_AND_EXIT(string);
    }
}

// Returns the texture size scale factor between corresponding TEXT and TEXD resources.
// dimension(TEXD) / scale_factor == dimensions(TEXT)
// adapted from pawREP/GlacierFormats
size_t Texture::getScaleFactor(int texdW, int texdH)
{
    switch (texdW * texdH)
    {
    case 4096:
        return 1;
    case 32768:
    case 65536:
        return 2;
    case 131072:
    case 262144:
        return 4;
    case 524288:
    case 1048576:
        return 8;
    case 2097152:
    case 4194304:
        return 16;
    case 8388608:
    case 16777216:
        return 32;
    default:
        LOG("[w] TEXT Scale Factor could not be found, defaulting to 1, this may cause issues!");
        return 1;
    }
}

// Returns max number of mips supported for given image dimensions.
// adapted from pawREP/GlacierFormats
size_t Texture::maxMipsCount(size_t width, size_t height)
{
    size_t mipLevels = 1;

    while (height > 1 || width > 1)
    {
        if (height > 1)
            height >>= 1;

        if (width > 1)
            width >>= 1;

        ++mipLevels;
    }

    // Just in case, never actually reached this
    if (mipLevels > 0xE)
        return 0xE;

    return mipLevels;
}

// Returns total number of bytes required to contain an image with the given dimensions, mips levels and format.
// adapted from pawREP/GlacierFormats
size_t Texture::getTotalPixelsSize(int width, size_t height, int mipsLevels, DXGI_FORMAT format)
{
    size_t totalPixelSize = 0;

    for (size_t level = 0; level < mipsLevels; ++level)
    {
        size_t rowPitch, slicePitch;
        HRESULT hr = DirectX::ComputePitch(format, width, height, rowPitch, slicePitch, DirectX::CP_FLAGS_NONE);
        if (FAILED(hr))
            handleHRESULT("Unexpected failure in ComputePitch", hr);

        totalPixelSize += uint64_t(slicePitch);

        if (height > 1)
            height >>= 1;

        if (width > 1)
            width >>= 1;
    }
    return totalPixelSize;
}

// Gets the pixel block size of the format
size_t Texture::getPixelBlockSize(Format format)
{
    switch (format)
    {
    // Only for block compressed formats
    case Texture::Format::DXT1:
    case Texture::Format::DXT5:
    case Texture::Format::BC4:
    case Texture::Format::BC5:
    case Texture::Format::BC7:
        return 4;
    default:
        return 1;
    }
}

// Converts Texture::Format to DXGI_FORMAT_XXX
// adapted from pawREP/GlacierFormats
DXGI_FORMAT Texture::toDxgiFormat(Format format)
{
    switch (format)
    {
    case Texture::Format::R8G8B8A8:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Texture::Format::A8:
        return DXGI_FORMAT_A8_UNORM;
    case Texture::Format::R8G8:
        return DXGI_FORMAT_R8G8_UNORM;
    case Texture::Format::DXT1:
        return DXGI_FORMAT_BC1_UNORM;
    case Texture::Format::DXT5:
        return DXGI_FORMAT_BC3_UNORM;
    case Texture::Format::BC4:
        return DXGI_FORMAT_BC4_UNORM;
    case Texture::Format::BC5:
        return DXGI_FORMAT_BC5_UNORM;
    case Texture::Format::BC7:
        return DXGI_FORMAT_BC7_UNORM;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

// Since we have varying header formats, we have to pass values directly
// The function has been modified to follow the formatting of the project
// along with adding R8G8B8A8 and returning HRESULTs
// Majority of the function is from pawREP/GlacierFormats
HRESULT Texture::createDDS(Format format, uint32_t width, uint32_t height, uint32_t mipCount, std::vector<char> pixels, DirectX::Blob &blob)
{
    DirectX::DDS_HEADER ddsHeader{};
    DirectX::DDS_HEADER_DXT10 ddsHeaderDXT10{};

    ddsHeader.size = 124;
    ddsHeader.flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    ddsHeader.height = height;
    ddsHeader.width = width;
    ddsHeader.pitchOrLinearSize = 0; //as per doc, most programs ignore this value anyway.
    ddsHeader.depth = 0;
    ddsHeader.mipMapCount = mipCount;
    std::fill(ddsHeader.reserved1, &ddsHeader.reserved1[11], 0);
    ddsHeader.caps = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
    ddsHeader.caps2 = NULL;
    ddsHeader.caps3 = NULL;
    ddsHeader.caps4 = NULL;
    ddsHeader.reserved2 = NULL;

    switch (format)
    {
    case Texture::Format::R8G8B8A8:
        ddsHeader.ddspf = DirectX::DDSPF_A8B8G8R8;
        break;
    case Texture::Format::R8G8:
        ddsHeader.ddspf = DirectX::DDSPF_R8G8_B8G8;
        break;
    case Texture::Format::A8:
        ddsHeader.ddspf = DirectX::DDSPF_A8;
        break;
    case Texture::Format::DXT1:
        ddsHeader.ddspf = DirectX::DDSPF_DXT1;
        break;
    case Texture::Format::DXT5:
        ddsHeader.ddspf = DirectX::DDSPF_DXT5;
        break;
    case Texture::Format::BC4:
        ddsHeader.ddspf = DirectX::DDSPF_BC4_UNORM;
        break;
    case Texture::Format::BC5:
        ddsHeader.ddspf = DirectX::DDSPF_BC5_UNORM;
        break;
    case Texture::Format::BC7:
        ddsHeader.ddspf = DirectX::DDSPF_DX10;
        ddsHeaderDXT10.dxgiFormat = DXGI_FORMAT_BC7_UNORM;
        ddsHeaderDXT10.resourceDimension = DirectX::DDS_DIMENSION_TEXTURE2D;
        ddsHeaderDXT10.miscFlag = 0;
        ddsHeaderDXT10.arraySize = 1;
        ddsHeaderDXT10.miscFlags2 = 0;
        break;
    default:
        LOG_AND_EXIT("Invalid format reached in DDS header creation. Please report this to Anthony!");
    }

    size_t ddsFileBufferSize = 0;
    ddsFileBufferSize += sizeof(DirectX::DDS_MAGIC);
    ddsFileBufferSize += sizeof(DirectX::DDS_HEADER);
    if (format == Texture::Format::BC7)
        ddsFileBufferSize += sizeof(DirectX::DDS_HEADER_DXT10);
    ddsFileBufferSize += pixels.size();

    HRESULT hr = blob.Initialize(ddsFileBufferSize);
    if (FAILED(hr))
        return hr;

    char *ddsFileBuffer = reinterpret_cast<char *>(blob.GetBufferPointer());

    int bufOffset = 0; //TODO: Use BW
    memcpy_s(&ddsFileBuffer[bufOffset], sizeof(DirectX::DDS_MAGIC), &DirectX::DDS_MAGIC, sizeof(DirectX::DDS_MAGIC));
    bufOffset += sizeof(DirectX::DDS_MAGIC);

    memcpy_s(&ddsFileBuffer[bufOffset], sizeof(ddsHeader), &ddsHeader, sizeof(ddsHeader));
    bufOffset += sizeof(ddsHeader);

    if (format == Texture::Format::BC7)
    {
        memcpy_s(&ddsFileBuffer[bufOffset], sizeof(ddsHeaderDXT10), &ddsHeaderDXT10, sizeof(ddsHeaderDXT10));
        bufOffset += sizeof(ddsHeaderDXT10);
    }

    memcpy_s(&ddsFileBuffer[bufOffset], pixels.size(), pixels.data(), pixels.size());

    return S_OK;
}

// Compress textures. Add mipmap sizes and pixels
// adapted from pawREP/GlacierFormats, aspects of RPKG-Tool helped with compression
HRESULT Texture::compress(builtTexture &texture, DirectX::ScratchImage &mipChain, Format format, bool doCompression)
{
    HRESULT hr;
    DirectX::ScratchImage outImage;
    switch (format)
    {
    case Texture::Format::A8:
    case Texture::Format::R8G8:
    case Texture::Format::R8G8B8A8:
        outImage = std::move(mipChain);
        break;
    case Texture::Format::BC5:
        for (int i = 2; i < mipChain.GetPixelsSize(); i += 4)
            mipChain.GetPixels()[i] = 0xFF;
    case Texture::Format::DXT1:
    case Texture::Format::DXT5:
    case Texture::Format::BC4:
    case Texture::Format::BC7:
        hr = DirectX::Compress(EncodingDevice(), mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), toDxgiFormat(format), DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, outImage);
        if (FAILED(hr))
        hr = DirectX::Compress(mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), toDxgiFormat(format), DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, outImage);
        if (FAILED(hr))
            return hr;
        break;
    }

    // Calculate new mipmap levels (sizes)
    const size_t blockSize = getPixelBlockSize(format);

    texture.mipsSizes[0] = DirectX::ComputeScanlines(toDxgiFormat(format), outImage.GetImage(0, 0, 0)->height) * outImage.GetImage(0, 0, 0)->rowPitch;
    for (int i = 1; i < outImage.GetImageCount(); i++)
        texture.mipsSizes[i] = texture.mipsSizes[i - 1] +
                               DirectX::ComputeScanlines(toDxgiFormat(format), outImage.GetImage(i, 0, 0)->height) * outImage.GetImage(i, 0, 0)->rowPitch;

    if (doCompression)
    {
        uint32_t compressedBound = LZ4_compressBound(outImage.GetPixelsSize());
        texture.compressedPixels.reserve(compressedBound);
        uint32_t totalCompressedSize = 0;
        for (int i = 0; i < outImage.GetImageCount(); i++)
        {
            std::vector<char> tempDest(compressedBound, 0);
            uint32_t compressedSize = LZ4_compress_HC((const char *)outImage.GetImage(i, 0, 0)->pixels, tempDest.data(), texture.mipsSizes[i] - texture.mipsSizes[i - 1], compressedBound, LZ4HC_CLEVEL_MAX);
            if (compressedSize == 0)
            {
                LOG_AND_EXIT("Failed to LZ4 compress mip block! Please report this to Anthony!");
            }

            totalCompressedSize += compressedSize;
            texture.compressedPixels.insert(texture.compressedPixels.end(), tempDest.begin(), tempDest.begin() + compressedSize);
            texture.compressedSizes[i] = totalCompressedSize;
        }
    }

    // Add pixels to the built texture
    texture.pixels.resize(outImage.GetPixelsSize());
    std::memcpy(texture.pixels.data(), outImage.GetPixels(), outImage.GetPixelsSize());

    return S_OK;
}

// This function has been simplified to be more concise
// Other than that, majority of the function is from pawREP/GlacierFormats
HRESULT Texture::outputToTGA(DirectX::Blob &blob, Format format, std::filesystem::path outputPath)
{
    DirectX::TexMetadata metadata;
    DirectX::ScratchImage origImage;
    DirectX::ScratchImage convImage;

    HRESULT hr = DirectX::LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DirectX::DDS_FLAGS_NONE, &metadata, origImage);
    if (FAILED(hr))
        return hr;

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
    bool fixChannel = false;
    switch (format)
    {
    case Texture::Format::A8:
    case Texture::Format::R8G8B8A8:
        convImage = std::move(origImage);
        break;
    case Texture::Format::R8G8:
        hr = DirectX::Convert(*origImage.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, convImage);
        if (FAILED(hr))
            return hr;

        fixChannel = true;
        break;
    case Texture::Format::BC5:
        fixChannel = true;
    case Texture::Format::DXT1:
    case Texture::Format::DXT5:
    case Texture::Format::BC7:
        dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case Texture::Format::BC4:
        dxgiFormat = DXGI_FORMAT_A8_UNORM;
        break;
    default:
        LOG_AND_EXIT("Unknown format in DDS conversion/decompression! Please report this to Anthony!");
        break;
    }

    if (dxgiFormat != DXGI_FORMAT_UNKNOWN)
    {
        hr = DirectX::Decompress(*origImage.GetImage(0, 0, 0), dxgiFormat, convImage);
        if (FAILED(hr))
            return hr;
    }

    if (fixChannel)
    {
        for (int i = 2; i < convImage.GetPixelsSize(); i += 4)
            convImage.GetPixels()[i] = 0xFF;
    }

    std::wstring wpath = outputPath.generic_wstring();
    hr = DirectX::SaveToTGAFile(*convImage.GetImage(0, 0, 0), DirectX::TGA_FLAGS_NONE, wpath.c_str(), nullptr);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

// This function has aspects from pawREP/GlacierFormats and glacier-modding/RPKG-Tool (for the mip sizes)
// Other than that, it's written from scratch
HRESULT Texture::import(std::filesystem::path tgaPath, Format format, bool rebuildBoth, bool isTEXD, bool doCompression, builtTexture &TEXT, builtTexture &TEXD)
{
    HRESULT hr;
    DirectX::TexMetadata meta;
    DirectX::ScratchImage inputImage;
    DirectX::ScratchImage mipChain;

    LOG("Loading TGA...");
    std::wstring wpath = tgaPath.generic_wstring();
    hr = DirectX::LoadFromTGAFile(wpath.c_str(), DirectX::TGA_FLAGS_NONE, &meta, inputImage);
    if (FAILED(hr))
        return hr;

    switch (format)
    {
    case Texture::Format::R8G8:
        DirectX::ScratchImage tempImg{};
        hr = DirectX::Convert(*inputImage.GetImage(0, 0, 0), toDxgiFormat(format), DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, tempImg);
        if (FAILED(hr))
            return hr;

        inputImage = std::move(tempImg);
        break;
    }

    if (rebuildBoth)
    {
        LOG("Resizing TGA for TEXT [Rebuild Both]...");
        DirectX::ScratchImage textInput;
        DirectX::ScratchImage textMipChain;

        size_t width = inputImage.GetImage(0, 0, 0)->width;
        size_t height = inputImage.GetImage(0, 0, 0)->height;

        size_t mipCount = maxMipsCount(width, height);
        size_t sf = getScaleFactor(width, height);
        size_t textMipCount = maxMipsCount(width / sf, height / sf); // TEXT with a TEXD have a different number of mip maps

        hr = DirectX::Resize(*inputImage.GetImage(0, 0, 0), width / sf, height / sf, DirectX::TEX_FILTER_DEFAULT, textInput);
        if (FAILED(hr))
            return hr;

        LOG("Generating resized TEXT mipmaps...");
        hr = DirectX::GenerateMipMaps(*textInput.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, textMipCount, textMipChain);
        if (FAILED(hr))
            return hr;

        TEXT.width = textMipChain.GetImage(0, 0, 0)->width;
        TEXT.height = textMipChain.GetImage(0, 0, 0)->height;
        TEXT.mipsCount = textMipCount;

        LOG("Compressing TEXT...");
        hr = compress(TEXT, textMipChain, format, doCompression);
        if (FAILED(hr))
            return hr;

        LOG("Generating TEXD mipmaps...");
        hr = DirectX::GenerateMipMaps(*inputImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, mipCount, mipChain);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        if (isTEXD)
        {
            LOG("Generating mipmaps [Only TEXD]...");
            size_t mipCount = maxMipsCount(inputImage.GetImage(0, 0, 0)->width, inputImage.GetImage(0, 0, 0)->height);
            hr = DirectX::GenerateMipMaps(*inputImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, mipCount, mipChain);
            if (FAILED(hr))
                return hr;
        }
        else
        {
            // For TEXT only textures, use the number
            // of images in the input image (usually 1)
            mipChain = std::move(inputImage);
        }
    }

    ((isTEXD || rebuildBoth) ? TEXD : TEXT).width = mipChain.GetImage(0, 0, 0)->width;
    ((isTEXD || rebuildBoth) ? TEXD : TEXT).height = mipChain.GetImage(0, 0, 0)->height;
    ((isTEXD || rebuildBoth) ? TEXD : TEXT).mipsCount = mipChain.GetImageCount();

    LOG("Compressing " << ((isTEXD || rebuildBoth) ? "TEXD" : "TEXT") << "...");
    // Compress the TEXD if it's on its own or if you're rebuilding both
    hr = compress((isTEXD || rebuildBoth) ? TEXD : TEXT, mipChain, format, doCompression);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

std::string Texture::versionToString(Version version)
{
    switch (version)
    {
    case Version::H2016_A:
        return "H2016 Alpha";
    case Version::H2016:
        return "H2016";
    case Version::H2:
        return "H2";
    case Version::H3:
        return "H3";
    case Version::HMA:
        return "HMA";
    default:
        return "NONE";
    }
}

std::vector<char> Texture::PS4swizzle(std::vector<char> &data, Format format, uint16_t width, uint16_t height, bool deswizzle)
{
    LOG("[PS4] " << (deswizzle ? "Deswizzling" : "Swizzling") << " texture...");
    std::vector<char> deswizzled{};
    deswizzled.resize(data.size());

    int pixelBlockSize = getPixelBlockSize(format);
    width /= pixelBlockSize;
    height /= pixelBlockSize;

    int bytesPerPixel = DirectX::BitsPerPixel(toDxgiFormat(format)) * 2;
    if (pixelBlockSize == 1)
        bytesPerPixel = (bytesPerPixel / 2) / 8;

    int index = 0;
    for (int y = 0; y < (height + 7) / 8; ++y)
    {
        for (int x = 0; x < (width + 7) / 8; ++x)
        {
            for (int t = 0; t < 64; ++t)
            {
                uint_fast16_t XpixelIndex = 8;
                uint_fast16_t YpixelIndex = 8;
                libmorton::morton2D_32_decode(t, XpixelIndex, YpixelIndex);
                int pixelIndex = YpixelIndex * 8 + XpixelIndex;

                int xOffset = (x * 8) + (pixelIndex % 8);
                int yOffset = (y * 8) + (pixelIndex / 8);

                if (xOffset < width && yOffset < height)
                {
                    int destIndex = bytesPerPixel * (yOffset * width + xOffset);

                    if (deswizzle)
                        std::memcpy(&deswizzled.at(destIndex), &data.at(index), bytesPerPixel);
                    else
                        std::memcpy(&deswizzled.at(index), &data.at(destIndex), bytesPerPixel);
                }

                index += bytesPerPixel;
            }
        }
    }

    return deswizzled;
}

bool Texture::writeFile(void *buffer, size_t size, std::filesystem::path path)
{
    std::ofstream FILE(path.generic_string(), std::ios::out | std::ofstream::binary);
    if (!FILE.good())
    {
        LOG_AND_EXIT("Could not open write stream to file!");
    }

    FILE.write((const char *)buffer, size);
    FILE.close();

    return true;
}

template <typename T>
T Texture::readMetaFile(std::filesystem::path path)
{
    // Check file exists
    if (!std::filesystem::exists(path.generic_string()))
    {
        LOG_AND_EXIT("Could not find the meta file associated with this texture! Please make sure it exists.");
    }

    // Load file data into vector
    std::ifstream FILE(path.generic_string(), std::ifstream::binary);
    if (!FILE.good())
    {
        LOG_AND_EXIT("Could not open read stream to file!");
    }
    std::vector<char> fileData((std::istreambuf_iterator<char>(FILE)), std::istreambuf_iterator<char>());
    FILE.close();

    if (fileData.size() != sizeof(T))
    {
        LOG_AND_EXIT("Meta file size mismatch! Please make sure it is a valid meta file for this version!");
    }

    T meta{};
    std::memcpy(&meta, &fileData[0], sizeof(meta));

    return meta;
}

template <typename T>
bool Texture::writeTexture(T texture, std::filesystem::path path)
{
    std::ofstream FILE(path.generic_string(), std::ios::out | std::ofstream::binary);
    if (!FILE.good())
    {
        LOG_AND_EXIT("Could not open write stream to file!");
    }

    FILE.write((const char *)&texture.header, sizeof(texture.header));
    FILE.write((const char *)&texture.pixels[0], texture.pixels.size());

    FILE.close();

    return true;
}

// HMA Functions
bool Texture::HMA::readHeader(std::vector<char> textureData, Header &header)
{
    std::memcpy(&header, &textureData[0], sizeof(header));

    assert_msg(header.magic == 1, "Invalid texture magic! Please make sure it is an actual texture!");

    assert_msg(header.dimensions == 0, "Unknown dimensions! Please report this to Anthony!");

    return true;
}

void Texture::HMA::Convert(std::vector<char> textureData, std::string outputPath, bool ps4swizzle, std::string metaPath)
{
    Texture::HMA::TEXT texture{};

    readHeader(textureData, texture.header);

    if (toDxgiFormat(texture.header.format) == DXGI_FORMAT_UNKNOWN)
    {
        LOG_AND_EXIT("Invalid texture format found. Please report this to Anthony!");
    }

    texture.pixels.resize(textureData.size() - 0x20);
    std::memcpy(&texture.pixels[0], &textureData[0x20], (textureData.size() - 0x20));

    DirectX::Blob ddsBuffer{};
    uint16_t width = texture.header.width;
    uint16_t height = texture.header.height;
    size_t mips = texture.header.mipsCount - 1;

    if (ps4swizzle)
        texture.pixels = Texture::PS4swizzle(texture.pixels, texture.header.format, width, height, true);

    HRESULT hr = createDDS(texture.header.format, width, height, mips, texture.pixels, ddsBuffer);
    if (FAILED(hr))
        handleHRESULT("Failed to create DDS!", hr);

    std::filesystem::path outPath = outputPath;
    hr = outputToTGA(ddsBuffer, texture.header.format, outPath);
    if (FAILED(hr))
        handleHRESULT("Failed to output to TGA!", hr);

    HMA::Meta meta = {
        texture.header.type,
        texture.header.flags,
        texture.header.format,
        texture.header.interpretAs,
        texture.header.dimensions,
        texture.header.mipsInterpolMode};

    LOG("Outputting meta...");
    writeFile(&meta, sizeof(meta), metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta"));
    LOG("Finished outputting meta");

    LOG("Converted HMA texture to TGA successfully!");
}

void Texture::HMA::Rebuild(std::string tgaPath, std::string outputPath, bool ps4swizzle, std::string metaPath)
{
    HRESULT hr;
    Texture::HMA::TEXT TEXT{};

    LOG("Reading meta file...");
    HMA::Meta meta = readMetaFile<HMA::Meta>(metaPath != "" ? metaPath : tgaPath + ".tonymeta");

    builtTexture builtTEXT{};
    builtTexture builtTEXD{};
    hr = import(tgaPath, meta.format, false, true, false, builtTEXT, builtTEXD);
    if (FAILED(hr))
        handleHRESULT("Failed to import from TGA!", hr);

    uint32_t filesize = (sizeof(HMA::Header) - 4) + builtTEXD.pixels.size();

    TEXT.header = {
        1,
        meta.type,
        filesize,
        meta.flags,
        builtTEXD.width,
        builtTEXD.height,
        meta.format,
        builtTEXD.mipsCount,
        0,
        meta.interpretAs,
        meta.dimensions,
        meta.mipsInterpolMode,
        0,
        filesize};

    TEXT.pixels = std::move(builtTEXD.pixels);

    LOG("Writing TEXT...");
    writeTexture<HMA::TEXT>(TEXT, outputPath);

    LOG("Finished rebuilding TGA to TEXT.");
}

// H2016 Functions
bool Texture::H2016::readHeader(std::vector<char> textureData, Header &header, bool isTEXD = false)
{
    std::memcpy(&header, &textureData[0], sizeof(header));

    if (header.texdIdentifier == 16384 && !isTEXD)
    {
        LOG("[w] Input texture identified to be TEXT that has a TEXD or is a TEXD. You have not specified that this is a TEXD!");
        LOG("[w] This can cause issues if it actually is a TEXD so you have been warned! [If this is incorrect please let Anthony know!]");
    }

    assert_msg(header.magic == 1, "Invalid texture magic! Please make sure it is an actual texture!");

    assert_msg(header.defaultMip == 0, "Unknown default mip! Please report this to Anthony!");
    assert_msg(header.dimensions == 0, "Unknown dimensions! Please report this to Anthony!");
    assert_msg(header.mipsInterpolMode == 0, "Unknown interpol mode! Please report this to Anthony!");

    assert_msg(header.textAtlasSize == 0, "Texture atlas found! There are not yet supported.");
    assert_msg(header.textAtlasOffset == 0x54, "Texture atlas found! There are not yet supported.");

    return true;
}

void Texture::H2016::Convert(std::vector<char> textureData, std::string outputPath, bool ps4swizzle, Version portTo, bool isTEXD, std::string texdOutput, std::string metaPath)
{
    Texture::H2016::TEXT texture{};

    readHeader(textureData, texture.header, isTEXD);

    if (toDxgiFormat(texture.header.format) == DXGI_FORMAT_UNKNOWN)
    {
        LOG_AND_EXIT("Invalid texture format found. Please report this to Anthony!");
    }

    texture.pixels.resize(textureData.size() - 0x5C);
    std::memcpy(&texture.pixels[0], &textureData[0x5C], (textureData.size() - 0x5C));

    DirectX::Blob ddsBuffer{};
    uint16_t width = texture.header.width;
    uint16_t height = texture.header.height;
    size_t mips = texture.header.mipsCount;

    if (!isTEXD)
    {
        size_t sf = getScaleFactor(texture.header.width, texture.header.height);
        width /= sf;
        height /= sf;

        if (texture.header.texdIdentifier == 16384)
        {
            mips = maxMipsCount(width, height);
        }
    }

    if (ps4swizzle)
    {
        texture.pixels = Texture::PS4swizzle(texture.pixels, texture.header.format, width, height, true);
    }

    HRESULT hr = createDDS(texture.header.format, width, height, mips, texture.pixels, ddsBuffer);
    if (FAILED(hr))
        handleHRESULT("Failed to create DDS!", hr);

    std::filesystem::path outPath = outputPath;
    if (portTo != Version::NONE)
        outPath += ".temp.tga";

    hr = outputToTGA(ddsBuffer, texture.header.format, outPath);
    if (FAILED(hr))
        handleHRESULT("Failed to output to TGA!", hr);

    if (ps4swizzle)
        LOG("[PS4] As a PS4 swizzle has been specified, the flag has been altered.");

    H2016::Meta meta = {
        texture.header.type,
        texture.header.flags - (ps4swizzle ? 1 : 0),
        texture.header.format,
        texture.header.interpretAs};

    LOG("Outputting meta...");
    writeFile(&meta, sizeof(meta), metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta"));
    LOG("Finished outputting meta");

    if (portTo != Version::NONE)
    {
        LOG("Porting to " + versionToString(portTo));

        std::filesystem::path portPath = outputPath;
        switch (portTo)
        {
        case Version::H2016:
            H2016::Rebuild(outPath.generic_string(), portPath.generic_string(), false, isTEXD, ps4swizzle, texdOutput, metaPath);
            break;
        case Version::H2:
            H2::Rebuild(outPath.generic_string(), portPath.generic_string(), false, isTEXD, ps4swizzle, texdOutput, metaPath);
            break;
        default:
            LOG_AND_EXIT("Invalid port to location!");
        }

        remove(outPath.generic_string().c_str());
        remove((metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta")).c_str());
        LOG_AND_EXIT("Ported!");
    }
    else
    {
        LOG("Converted H2016 texture to TGA successfully!");
    }
}

void Texture::H2016::Rebuild(std::string tgaPath, std::string outputPath, bool rebuildBoth, bool isTEXD, bool ps4swizzle, std::string texdOutput, std::string metaPath)
{
    HRESULT hr;
    Texture::H2016::TEXT TEXT{};
    Texture::H2016::TEXT TEXD{};

    LOG("Reading meta file...");
    H2016::Meta meta = readMetaFile<H2016::Meta>(metaPath != "" ? metaPath : (tgaPath + ".tonymeta"));

    builtTexture builtTEXT{};
    builtTexture builtTEXD{};
    hr = import(tgaPath, meta.format, rebuildBoth, isTEXD, false, builtTEXT, builtTEXD);
    if (FAILED(hr))
        handleHRESULT("Failed to import from TGA!", hr);

    if (rebuildBoth)
    {
        // -8 as it does not count the magic and type
        uint32_t filesize = (sizeof(H2016::Header) - 8) + builtTEXD.pixels.size();

        TEXD.header = {
            1,
            meta.type,
            16384,
            filesize,
            meta.flags,
            builtTEXD.width,
            builtTEXD.height,
            meta.format,
            builtTEXD.mipsCount};

        for (int i = 0; i < 0xE; i++)
            TEXD.header.mipsDataSizes[i] = builtTEXD.mipsSizes[i];

        TEXD.header.textAtlasOffset = 0x54;
        TEXT.header = TEXD.header;
        TEXD.pixels = std::move(builtTEXD.pixels);
        TEXT.pixels = std::move(builtTEXT.pixels);

        LOG("Writing TEXT and TEXD...");
        writeTexture<H2016::TEXT>(TEXT, texdOutput == "" ? outputPath + ".TEXT" : outputPath);
        writeTexture<H2016::TEXT>(TEXD, texdOutput == "" ? outputPath + ".TEXD" : texdOutput);

        LOG("Finished rebuilding TGA to TEXT and TEXD.");
    }
    else
    {
        if (isTEXD)
        {
            // -8 as it does not count the magic and type
            uint32_t filesize = (sizeof(H2016::Header) - 8) + builtTEXD.pixels.size();
            sizeof(Format);

            TEXD.header = {
                1,
                meta.type,
                16384,
                filesize,
                meta.flags,
                builtTEXD.width,
                builtTEXD.height,
                meta.format,
                builtTEXD.mipsCount,
                0,
                meta.interpretAs};

            for (int i = 0; i < 0xE; i++)
                TEXD.header.mipsDataSizes[i] = builtTEXD.mipsSizes[i];

            TEXD.header.textAtlasOffset = 0x54;
            TEXD.pixels = std::move(builtTEXD.pixels);

            LOG("Writing TEXD...");
            writeTexture<H2016::TEXT>(TEXD, outputPath);

            LOG("Finished rebuilding TGA to TEXD.");
        }
        else
        {
            // -8 as it does not count the magic and type
            uint32_t filesize = (sizeof(H2016::Header) - 8) + builtTEXT.pixels.size();
            sizeof(Format);

            TEXT.header = {
                1,
                meta.type,
                0xFFFFFFFF,
                filesize,
                meta.flags,
                builtTEXT.width,
                builtTEXT.height,
                meta.format,
                builtTEXT.mipsCount};

            for (int i = 0; i < 0xE; i++)
                TEXT.header.mipsDataSizes[i] = builtTEXT.mipsSizes[i];

            TEXT.header.textAtlasOffset = 0x54;
            TEXT.pixels = std::move(builtTEXT.pixels);

            LOG("Writing TEXT...");
            writeTexture<H2016::TEXT>(TEXT, outputPath);

            LOG("Finished rebuilding TGA to TEXT.");
        }
    }
}

// H2 Functions
bool Texture::H2::readHeader(std::vector<char> textureData, Header &header, bool isTEXD = false)
{
    std::memcpy(&header, &textureData[0], sizeof(header));

    if (header.texdIdentifier == 16384 && !isTEXD)
    {
        LOG("[w] Input texture identified to be TEXT that has a TEXD or is a TEXD. You have not specified that this is a TEXD!");
        LOG("[w] This can cause issues if it actually is a TEXD so you have been warned! [If this is incorrect please let Anthony know!]");
    }

    assert_msg(header.magic == 1, "Invalid texture magic! Please make sure it is an actual texture!");

    assert_msg(header.defaultMip == 1, "Unknown default mip! Please report this to Anthony!");

    assert_msg(header.textAtlasSize == 0, "Texture atlas found! There are not yet supported.");
    assert_msg(header.textAtlasOffset == 0x90, "Texture atlas found! There are not yet supported.");

    return true;
}

void Texture::H2::Convert(std::vector<char> textureData, std::string outputPath, bool ps4swizzle, Version portTo, bool isTEXD, std::string texdOutput, std::string metaPath)
{
    Texture::H2::TEXT texture{};

    readHeader(textureData, texture.header, isTEXD);

    if (toDxgiFormat(texture.header.format) == DXGI_FORMAT_UNKNOWN)
        LOG_AND_EXIT("Invalid texture format found. Please report this to Anthony!");

    texture.pixels.resize(textureData.size() - 0x90);
    std::memcpy(&texture.pixels[0], &textureData[0x90], (textureData.size() - 0x90));

    DirectX::Blob ddsBuffer{};
    uint16_t width = texture.header.width;
    uint16_t height = texture.header.height;
    size_t mips = texture.header.mipsCount;

    if (!isTEXD)
    {
        size_t sf = getScaleFactor(texture.header.width, texture.header.height);
        width /= sf;
        height /= sf;

        if (texture.header.texdIdentifier == 16384)
            mips = maxMipsCount(width, height);
    }

    if (ps4swizzle)
        texture.pixels = Texture::PS4swizzle(texture.pixels, texture.header.format, width, height, true);

    HRESULT hr = createDDS(texture.header.format, width, height, mips, texture.pixels, ddsBuffer);
    if (FAILED(hr))
        handleHRESULT("Failed to create DDS!", hr);

    std::filesystem::path outPath = outputPath;
    if (portTo != Version::NONE)
        outPath += ".temp.tga";

    hr = outputToTGA(ddsBuffer, texture.header.format, outPath);
    if (FAILED(hr))
        handleHRESULT("Failed to output to TGA!", hr);

    if (ps4swizzle)
        LOG("[PS4] As a PS4 swizzle has been specified, the flag has been altered.");

    H2::Meta meta{
        texture.header.type,
        texture.header.flags - (ps4swizzle ? 1 : 0),
        texture.header.format};

    LOG("Outputting meta...");
    writeFile(&meta, sizeof(meta), metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta"));
    LOG("Finished outputting meta");

    if (portTo != Version::NONE)
    {
        LOG("Porting to " + versionToString(portTo));

        std::filesystem::path portPath = outputPath;
        switch (portTo)
        {
        case Version::H2016:
            H2016::Rebuild(outPath.generic_string(), portPath.generic_string(), false, isTEXD, ps4swizzle, texdOutput, metaPath);
            break;
        case Version::H2:
            H2::Rebuild(outPath.generic_string(), portPath.generic_string(), false, isTEXD, ps4swizzle, texdOutput, metaPath);
            break;
        default:
            LOG_AND_EXIT("Invalid port to location!");
        }

        remove(outPath.generic_string().c_str());
        remove((metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta")).c_str());
        LOG_AND_EXIT("Ported!");
    }
    else
    {
        LOG("Converted H2 texture to TGA successfully!");
    }
}

void Texture::H2::Rebuild(std::string tgaPath, std::string outputPath, bool rebuildBoth, bool isTEXD, bool ps4swizzle, std::string texdOutput, std::string metaPath)
{
    HRESULT hr;
    Texture::H2::TEXT TEXT{};
    Texture::H2::TEXT TEXD{};

    LOG("Reading meta file...");
    H2::Meta meta = readMetaFile<H2::Meta>(metaPath != "" ? metaPath : (tgaPath + ".tonymeta"));

    builtTexture builtTEXT{};
    builtTexture builtTEXD{};
    hr = import(tgaPath, meta.format, rebuildBoth, isTEXD, false, builtTEXT, builtTEXD);
    if (FAILED(hr))
        handleHRESULT("Failed to import from TGA!", hr);

    if (rebuildBoth)
    {
        uint32_t filesize = sizeof(H2::Header) + builtTEXD.pixels.size();

        TEXD.header = {
            1,
            meta.type,
            filesize,
            meta.flags,
            builtTEXD.width,
            builtTEXD.height,
            meta.format,
            builtTEXD.mipsCount,
            1,
            16384};

        for (int i = 0; i < 0xE; i++)
        {
            TEXD.header.mipsDataSizes[i] = builtTEXD.mipsSizes[i];
            TEXD.header.mipsDataSizesDup[i] = builtTEXD.mipsSizes[i];
        }

        TEXD.header.textAtlasOffset = 0x90;
        TEXT.header = TEXD.header;
        TEXD.pixels = std::move(builtTEXD.pixels);
        TEXT.pixels = std::move(builtTEXT.pixels);

        LOG("Writing TEXT and TEXD...");
        writeTexture<H2::TEXT>(TEXT, texdOutput == "" ? outputPath + ".TEXT" : outputPath);
        writeTexture<H2::TEXT>(TEXD, texdOutput == "" ? outputPath + ".TEXD" : texdOutput);

        LOG("Finished rebuilding TGA to TEXT and TEXD.");
    }
    else
    {
        if (isTEXD)
        {
            // -8 as it does not count the magic and type
            uint32_t filesize = (sizeof(H2016::Header) - 8) + builtTEXD.pixels.size();
            sizeof(Format);

            TEXD.header = {
                1,
                meta.type,
                filesize,
                meta.flags,
                builtTEXD.width,
                builtTEXD.height,
                meta.format,
                builtTEXD.mipsCount,
                1,
                16384};

            for (int i = 0; i < 0xE; i++)
            {
                TEXD.header.mipsDataSizes[i] = builtTEXD.mipsSizes[i];
                TEXD.header.mipsDataSizesDup[i] = builtTEXD.mipsSizes[i];
            }

            TEXD.header.textAtlasOffset = 0x90;
            TEXD.pixels = std::move(builtTEXD.pixels);

            LOG("Writing TEXD...");
            writeTexture<H2::TEXT>(TEXD, outputPath);

            LOG("Finished rebuilding TGA to TEXD.");
        }
        else
        {
            uint32_t filesize = sizeof(H2::Header) + builtTEXT.pixels.size();
            sizeof(Format);

            TEXT.header = {
                1,
                meta.type,
                filesize,
                meta.flags,
                builtTEXT.width,
                builtTEXT.height,
                meta.format,
                builtTEXT.mipsCount,
                1,
                0xFFFFFFFF};

            for (int i = 0; i < 0xE; i++)
            {
                TEXT.header.mipsDataSizes[i] = builtTEXT.mipsSizes[i];
                TEXT.header.mipsDataSizesDup[i] = builtTEXT.mipsSizes[i];
            }

            TEXT.header.textAtlasOffset = 0x90;
            TEXT.pixels = std::move(builtTEXT.pixels);

            LOG("Writing TEXT...");
            writeTexture<H2::TEXT>(TEXT, outputPath);

            LOG("Finished rebuilding TGA to TEXT.");
        }
    }
}

// H3 Functions
bool Texture::H3::readHeader(std::vector<char> textureData, Header &header)
{
    std::memcpy(&header, &textureData[0], sizeof(header));

    assert_msg(header.magic == 1, "Invalid texture magic! Please make sure it is an actual texture!");

    assert_msg(header.defaultMip == 0, "Unknown default mip! Please report this to Anthony!");
    assert_msg(header.interpretAs == 1, "Unknown interpret as! Please report this to Anthony!");
    assert_msg(header.dimensions == 0, "Unknown dimensions! Please report this to Anthony!");
    assert_msg(header.mipsInterpolMode == 0, "Unknown interpol mode! Please report this to Anthony!");

    assert_msg(header.textAtlasSize == 0, "Texture atlas found! There are not yet supported.");
    assert_msg(header.textAtlasOffset == 0x98, "Texture atlas found! There are not yet supported.");

    return true;
}

void Texture::H3::Convert(std::vector<char> textData, std::vector<char> texdData, std::string outputPath, bool ps4swizzle, Version portTo, bool hasTEXD, std::string texdOutput, std::string metaPath)
{
    HRESULT hr;
    Texture::H3::TEXT TEXT{};
    Texture::H3::TEXD TEXD{};

    readHeader(textData, TEXT.header);

    TEXT.pixels.resize(textData.size() - TEXT.header.textAtlasOffset);
    std::memcpy(&TEXT.pixels[0], &textData[TEXT.header.textAtlasOffset], (textData.size() - TEXT.header.textAtlasOffset));

    if (hasTEXD)
        TEXD.pixels = std::move(texdData);

    DirectX::Blob ddsBuffer{};
    uint16_t width = TEXT.header.width;
    uint16_t height = TEXT.header.height;
    size_t mips = TEXT.header.mipsCount;
    bool isCompressed = false;

    uint32_t widthSF = (2 << (TEXT.header.textScalingWidth - 1));
    uint32_t heightSF = (2 << (TEXT.header.textScalingHeight - 1));
    uint32_t texdScale = widthSF * heightSF;

    if (hasTEXD)
    {
        if (TEXT.header.texdBlockSizes[0] > 0 && TEXT.header.texdMipsSizes[0] != TEXT.header.texdBlockSizes[0])
        {
            isCompressed = true;
            size_t maxSize = getTotalPixelsSize(width, height, mips, toDxgiFormat(TEXT.header.format));

            std::vector<char> tempTEXDpixels(maxSize, 0);
            LZ4_decompress_safe(TEXD.pixels.data(), tempTEXDpixels.data(), TEXD.pixels.size(), maxSize);
            TEXD.pixels = std::move(tempTEXDpixels);
        }

        if (ps4swizzle)
            TEXD.pixels = Texture::PS4swizzle(TEXD.pixels, TEXT.header.format, width, height, true);

        hr = createDDS(TEXT.header.format, width, height, mips, TEXD.pixels, ddsBuffer);
        if (FAILED(hr))
            handleHRESULT("Failed to create DDS!", hr);
    }
    else
    {
        if (widthSF != 0 && heightSF != 0)
        {
            width /= widthSF;
            height /= heightSF;
        }

        if (TEXT.header.texdMipsSizes[0] != TEXT.header.texdBlockSizes[0])
        {
            isCompressed = true;
            size_t maxSize = getTotalPixelsSize(width, height, mips, toDxgiFormat(TEXT.header.format));

            std::vector<char> tempTEXTpixels(maxSize, 0);
            LZ4_decompress_safe(TEXT.pixels.data(), tempTEXTpixels.data(), TEXT.pixels.size(), maxSize);
            TEXT.pixels = std::move(tempTEXTpixels);
        }

        if (ps4swizzle)
            TEXT.pixels = Texture::PS4swizzle(TEXT.pixels, TEXT.header.format, width, height, true);

        hr = createDDS(TEXT.header.format, width, height, TEXT.header.textMipsLevels, TEXT.pixels, ddsBuffer);
        if (FAILED(hr))
            handleHRESULT("Failed to create DDS!", hr);
    }

    std::filesystem::path outPath = outputPath;
    if (portTo == Version::H3)
        outPath += ".temp.tga";

    hr = outputToTGA(ddsBuffer, TEXT.header.format, outPath);
    if (FAILED(hr))
        handleHRESULT("Failed to output to TGA!", hr);

    if (ps4swizzle)
        LOG("[PS4] As a PS4 swizzle has been specified, the flag has been altered.");

    H3::Meta meta{
        TEXT.header.type,
        TEXT.header.flags - (ps4swizzle ? 1 : 0),
        TEXT.header.format,
        isCompressed,
        TEXT.header.textScalingWidth,
        TEXT.header.textScalingHeight};

    LOG("Outputting meta...");
    writeFile(&meta, sizeof(meta), metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta"));
    LOG("Finished outputting meta");

    if (portTo == Version::H3)
    {
        LOG("Porting to " + versionToString(portTo));

        H3::Rebuild(outPath.generic_string(), outputPath, hasTEXD, false, texdOutput, metaPath);

        remove(outPath.generic_string().c_str());
        remove((metaPath != "" ? metaPath : (outPath.generic_string() + ".tonymeta")).c_str());
        LOG_AND_EXIT("Ported!");
    }
    else
    {
        LOG("Converted H3 texture to TGA successfully!");
    }
}

void Texture::H3::Rebuild(std::string tgaPath, std::string outputPath, bool rebuildBoth, bool ps4swizzle, std::string texdOutput, std::string metaPath)
{
    HRESULT hr;
    H3::TEXT TEXT{};
    H3::TEXD TEXD{};

    LOG("Reading meta file...");
    H3::Meta meta = readMetaFile<H3::Meta>(metaPath != "" ? metaPath : (tgaPath + ".tonymeta"));

    builtTexture builtTEXT{};
    builtTexture builtTEXD{};
    hr = import(tgaPath, meta.format, rebuildBoth, false, meta.isCompressed, builtTEXT, builtTEXD);
    if (FAILED(hr))
        handleHRESULT("Failed to import from TGA!", hr);

    if (rebuildBoth)
    {
        if (meta.isCompressed && (builtTEXD.mipsCount == builtTEXT.mipsCount))
        {
            TEXT.pixels = builtTEXT.compressedPixels;
            TEXD.pixels = std::move(builtTEXT.compressedPixels);
        }
        else if (meta.isCompressed)
        {
            TEXT.pixels = std::move(builtTEXT.compressedPixels);
            TEXD.pixels = std::move(builtTEXD.compressedPixels);
        }
        else
        {
            TEXD.pixels = std::move(builtTEXD.pixels);
            TEXT.pixels = std::move(builtTEXT.pixels);
        }

        uint32_t filesize = sizeof(H3::Header) + TEXD.pixels.size();

        TEXT.header = {
            1,
            meta.type,
            filesize,
            meta.flags,
            builtTEXD.width,
            builtTEXD.height,
            meta.format,
            builtTEXD.mipsCount};

        for (int i = 0; i < 0xE; i++)
        {
            TEXT.header.texdMipsSizes[i] = builtTEXD.mipsSizes[i];
            TEXT.header.texdBlockSizes[i] = (meta.isCompressed ? builtTEXD.compressedSizes : builtTEXD.mipsSizes)[i];
        }

        TEXT.header.textAtlasOffset = 0x98;

        TEXT.header.textScalingData1 = 0xFF;
        TEXT.header.textScalingWidth = meta.textScalingWidth; //((2 >> builtTEXT.width) + 1);
        TEXT.header.textScalingHeight = meta.textScalingHeight; //((2 >> builtTEXT.height) + 1);
        TEXT.header.textMipsLevels = builtTEXT.mipsCount;

        LOG("Writing TEXT and TEXD...");
        writeTexture<H3::TEXT>(TEXT, texdOutput == "" ? outputPath + ".TEXT" : outputPath);
        writeFile(TEXD.pixels.data(), TEXD.pixels.size(), texdOutput == "" ? outputPath + ".TEXD" : texdOutput);

        LOG("Finished rebuilding TGA to TEXT and TEXD.");
    }
    else
    {
        if (meta.isCompressed)
            TEXT.pixels = std::move(builtTEXT.compressedPixels);
        else
            TEXT.pixels = std::move(builtTEXT.pixels);

        uint32_t filesize = sizeof(H3::Header) + TEXT.pixels.size();

        TEXT.header = {
            1,
            meta.type,
            filesize,
            meta.flags,
            builtTEXT.width,
            builtTEXT.height,
            meta.format,
            builtTEXT.mipsCount};

        for (int i = 0; i < 0xE; i++)
        {
            TEXT.header.texdMipsSizes[i] = builtTEXT.mipsSizes[i];
            if (meta.isCompressed)
                TEXT.header.texdBlockSizes[i] = builtTEXT.compressedSizes[i];
            else
                TEXT.header.texdBlockSizes[i] = builtTEXT.mipsSizes[i];
        }

        TEXT.header.textAtlasOffset = 0x98;
        TEXT.header.textScalingData1 = 0xFF;
        TEXT.header.textMipsLevels = builtTEXT.mipsCount;

        LOG("Writing TEXT...");
        writeTexture<H3::TEXT>(TEXT, outputPath);

        LOG("Finished rebuilding TGA to TEXT.");
    }
}
