#include "main.h"
#include "EncodingDevice.h"

inline void handleHRESULT(std::string status, HRESULT hr)
{
    _com_error err(hr);
    LPCTSTR errMsg = err.ErrorMessage();

    LOG(status + " Please report this to Anthony!");
    LOG_AND_EXIT(errMsg);
}

void createDDS(std::vector<char> pixels, DirectX::Blob &blob)
{
    DirectX::DDS_HEADER ddsHeader{};
    DirectX::DDS_HEADER_DXT10 ddsHeaderDXT10{};

    ddsHeader.size = 124;
    ddsHeader.flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    ddsHeader.height = 768;
    ddsHeader.width = 128;
    ddsHeader.pitchOrLinearSize = 0; //as per doc, most programs ignore this value anyway.
    ddsHeader.depth = 0;
    ddsHeader.mipMapCount = 5;
    std::fill(ddsHeader.reserved1, &ddsHeader.reserved1[11], 0);
    ddsHeader.caps = DDS_SURFACE_FLAGS_TEXTURE | DDS_SURFACE_FLAGS_MIPMAP;
    ddsHeader.caps2 = NULL;
    ddsHeader.caps3 = NULL;
    ddsHeader.caps4 = NULL;
    ddsHeader.reserved2 = NULL;

    ddsHeader.ddspf = DirectX::DDSPF_DX10;
    ddsHeaderDXT10.dxgiFormat = DXGI_FORMAT_BC6H_UF16;
    ddsHeaderDXT10.resourceDimension = DirectX::DDS_DIMENSION_TEXTURE2D;
    ddsHeaderDXT10.miscFlag = 0;
    ddsHeaderDXT10.arraySize = 1;
    ddsHeaderDXT10.miscFlags2 = 0;

    size_t ddsFileBufferSize = 0;
    ddsFileBufferSize += sizeof(DirectX::DDS_MAGIC);
    ddsFileBufferSize += sizeof(DirectX::DDS_HEADER);
    ddsFileBufferSize += sizeof(DirectX::DDS_HEADER_DXT10);
    ddsFileBufferSize += pixels.size();

    HRESULT hr = blob.Initialize(ddsFileBufferSize);
    if (FAILED(hr))
        handleHRESULT("Failed to initialise DDS!", hr);

    char *ddsFileBuffer = reinterpret_cast<char *>(blob.GetBufferPointer());

    int bufOffset = 0; //TODO: Use BW
    memcpy_s(&ddsFileBuffer[bufOffset], sizeof(DirectX::DDS_MAGIC), &DirectX::DDS_MAGIC, sizeof(DirectX::DDS_MAGIC));
    bufOffset += sizeof(DirectX::DDS_MAGIC);

    memcpy_s(&ddsFileBuffer[bufOffset], sizeof(ddsHeader), &ddsHeader, sizeof(ddsHeader));
    bufOffset += sizeof(ddsHeader);


    memcpy_s(&ddsFileBuffer[bufOffset], sizeof(ddsHeaderDXT10), &ddsHeaderDXT10, sizeof(ddsHeaderDXT10));
    bufOffset += sizeof(ddsHeaderDXT10);

    memcpy_s(&ddsFileBuffer[bufOffset], pixels.size(), pixels.data(), pixels.size());
}

void outputToTGA(DirectX::Blob &blob, std::filesystem::path outputPath)
{
    DirectX::TexMetadata metadata;
    DirectX::ScratchImage origImage;
    DirectX::ScratchImage convImage;

    HRESULT hr = DirectX::LoadFromDDSMemory(blob.GetBufferPointer(), blob.GetBufferSize(), DirectX::DDS_FLAGS_NONE, &metadata, origImage);
    if (FAILED(hr))
        handleHRESULT("Failed to load DDS!", hr);

    hr = DirectX::Decompress(*origImage.GetImage(0, 0, 0), DXGI_FORMAT_R8G8B8A8_UNORM, convImage);
    if (FAILED(hr))
        handleHRESULT("Failed to decompress texture!", hr);
    
    std::wstring wpath = outputPath.generic_wstring();
    hr = DirectX::SaveToTGAFile(*convImage.GetImage(0, 0, 0), DirectX::TGA_FLAGS_NONE, wpath.c_str(), nullptr);
    if (FAILED(hr))
        handleHRESULT("Failed to save TGA!", hr);
}

std::vector<char> loadTGA(std::filesystem::path tgaPath)
{
    HRESULT hr;
    DirectX::TexMetadata meta;
    DirectX::ScratchImage inputImage;
    DirectX::ScratchImage mipChain;

    std::wstring wpath = tgaPath.generic_wstring();
    hr = DirectX::LoadFromTGAFile(wpath.c_str(), DirectX::TGA_FLAGS_NONE, &meta, inputImage);
    if (FAILED(hr))
        handleHRESULT("Failed to load TGA!", hr);

    if (inputImage.GetImage(0, 0, 0)->width != 128 || inputImage.GetImage(0, 0, 0)->height != 768)
    {
        LOG_AND_EXIT("Invalid image size! Must be 128x768!");
    }

    hr = DirectX::GenerateMipMaps(*inputImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 5, mipChain);
    if (FAILED(hr))
        handleHRESULT("Failed to generate mipmaps!", hr);

    DirectX::ScratchImage outImage;
    hr = DirectX::Compress(EncodingDevice(), mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), DXGI_FORMAT_BC6H_UF16, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, outImage);
        if (FAILED(hr))
            hr = DirectX::Compress(mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), DXGI_FORMAT_BC6H_UF16, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, outImage);
        if (FAILED(hr))
            handleHRESULT("Failed to compress image!", hr);

    std::vector<char> pixels;
    pixels.resize(outImage.GetPixelsSize());
    std::memcpy(pixels.data(), outImage.GetPixels(), outImage.GetPixelsSize());

    return pixels;
}