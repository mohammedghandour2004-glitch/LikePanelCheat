#pragma once

#include <d3d11.h>

ID3D11ShaderResourceView* LoadTextureFromFile(const char* path, ID3D11Device* device, int* outWidth, int* outHeight);
