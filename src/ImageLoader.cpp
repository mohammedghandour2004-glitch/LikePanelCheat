#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ID3D11ShaderResourceView* LoadTextureFromFile(const char* path, ID3D11Device* device, int* outWidth, int* outHeight)
{
    if (path == nullptr || device == nullptr)
    {
        return nullptr;
    }

    int imageWidth = 0;
    int imageHeight = 0;
    unsigned char* imageData = stbi_load(path, &imageWidth, &imageHeight, nullptr, 4);
    if (imageData == nullptr)
    {
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(imageWidth);
    desc.Height = static_cast<UINT>(imageHeight);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subresource = {};
    subresource.pSysMem = imageData;
    subresource.SysMemPitch = static_cast<UINT>(imageWidth * 4);

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &subresource, &texture);
    stbi_image_free(imageData);
    if (FAILED(hr) || texture == nullptr)
    {
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;

    ID3D11ShaderResourceView* textureView = nullptr;
    hr = device->CreateShaderResourceView(texture, &srvDesc, &textureView);
    texture->Release();
    if (FAILED(hr))
    {
        return nullptr;
    }

    if (outWidth != nullptr)
    {
        *outWidth = imageWidth;
    }
    if (outHeight != nullptr)
    {
        *outHeight = imageHeight;
    }

    return textureView;
}
