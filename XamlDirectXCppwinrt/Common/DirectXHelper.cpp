#include "pch.h"
#include <windows.graphics.directx.direct3d11.interop.h>

#pragma comment(lib, "D3D11.lib")


winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface CreateDepthTextureInteropObject(
	const winrt::com_ptr<ID3D11Texture2D> spTexture2D)
{
	// Direct3D interop APIs are used to provide the buffer to the WinRT API.
	auto depthStencilResource = spTexture2D.as<IDXGIResource1>();
	winrt::com_ptr<IDXGISurface2> depthDxgiSurface;
	winrt::check_hresult(depthStencilResource->CreateSubresourceSurface(0,
		depthDxgiSurface.put()));
	winrt::com_ptr<winrt::Windows::Foundation::IInspectable> inspectableSurface;
	winrt::check_hresult(
		CreateDirect3D11SurfaceFromDXGISurface(
			depthDxgiSurface.get(),
			(::IInspectable**)inspectableSurface.put()));

	return inspectableSurface.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface>();
}
