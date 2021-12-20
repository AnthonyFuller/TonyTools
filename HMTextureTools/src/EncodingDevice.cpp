#include "EncodingDevice.h"
#include "DirectXTex.h"

#include <d3d11.h>

#pragma comment(lib,"d3d11.lib")

ID3D11Device* EncodingDevice::device = nullptr;
ID3D11DeviceContext* EncodingDevice::context = nullptr;

EncodingDevice::operator ID3D11Device* () const noexcept {
	if (!device) {
#pragma warning( push )
#pragma warning( disable : 26812 )
		D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_11_0;
#pragma warning( pop )
		HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &feature_levels, 1, D3D11_SDK_VERSION, &device, NULL, &context);
		if (FAILED(hr)) {
			MessageBoxA(NULL, "Failed to initilize DX11 encoding device.", "Error", 0);
			exit(0);
		}
	}
	return device;
}