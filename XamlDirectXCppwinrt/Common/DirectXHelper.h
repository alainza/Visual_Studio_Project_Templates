#pragma once

namespace DX
{
    // Function that reads from a binary file asynchronously.
    inline winrt::Windows::Foundation::IAsyncAction ReadDataAsync(const std::wstring_view& filename, std::vector<byte>* vector)
    {
        using namespace winrt::Windows::Storage;
        using namespace winrt::Windows::Storage::Streams;

        IBuffer buffer = co_await PathIO::ReadBufferAsync(filename);

        vector->resize(buffer.Length());
        winrt::com_ptr<::Windows::Storage::Streams::IBufferByteAccess> byteAccess = buffer.as<::Windows::Storage::Streams::IBufferByteAccess>();
        byte* rawBuffer{ nullptr };
        winrt::check_hresult(byteAccess->Buffer(&rawBuffer));
        CopyMemory(vector->data(), rawBuffer, buffer.Length());
    }

    // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
    inline float ConvertDipsToPixels(float dips, float dpi)
    {
        constexpr float dipsPerInch = 96.0f;
        return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
    }

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface CreateDepthTextureInteropObject(
        const winrt::com_ptr<ID3D11Texture2D>& spTexture2D);

#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
            0,
            D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
            nullptr,                    // Any feature level will do.
            0,
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Runtime apps.
            nullptr,                    // No need to keep the D3D device reference.
            nullptr,                    // No need to know the feature level.
            nullptr                     // No need to keep the D3D device context reference.
        );

        return SUCCEEDED(hr);
    }
#endif
}
