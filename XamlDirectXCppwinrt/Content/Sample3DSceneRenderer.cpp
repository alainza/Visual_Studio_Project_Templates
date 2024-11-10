#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace winrt::$projectname$::implementation;

using namespace DirectX;
using namespace winrt::Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResourcesAsync();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	auto outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, 1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	auto vertexBufferNoRef = m_vertexBuffer.get();
	context->IASetVertexBuffers(
		0,
		1,
		&vertexBufferNoRef,
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer.get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	auto constantBufferNoRef = m_constantBuffer.get();
	context->VSSetConstantBuffers1(
		0,
		1,
		&constantBufferNoRef,
		nullptr,
		nullptr
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader.get(),
		nullptr,
		0
		);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}

winrt::fire_and_forget Sample3DSceneRenderer::CreateDeviceDependentResourcesAsync()
{
	// After the vertex shader file is loaded, create the shader and input layout.
	std::vector<byte> vertexShaderFileData;
	co_await DX::ReadDataAsync(L"ms-appx:///SampleVertexShader.cso", &vertexShaderFileData);

	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateVertexShader(
			&vertexShaderFileData[0],
			vertexShaderFileData.size(),
			nullptr,
			m_vertexShader.put()));

	static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateInputLayout(
			vertexDesc,
			ARRAYSIZE(vertexDesc),
			&vertexShaderFileData[0],
			vertexShaderFileData.size(),
			m_inputLayout.put()));

	// After the pixel shader file is loaded, create the shader and constant buffer.
	std::vector<byte> pixelShaderFileData;
	co_await DX::ReadDataAsync(L"ms-appx:///SamplePixelShader.cso", &pixelShaderFileData);
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreatePixelShader(
			&pixelShaderFileData[0],
			pixelShaderFileData.size(),
			nullptr,
			m_pixelShader.put()));

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			m_constantBuffer.put()));

	// Load mesh vertices. Each vertex has a position and a color.
	static const VertexPositionColor cubeVertices[] = 
	{
		// -x : orange
		{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.35f, 0.0f)},	 //  0
		{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.35f, 0.0f)},	 //  1
		{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 0.35f, 0.0f)},	 //  2
		{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 0.35f, 0.0f)},	 //  3

		// +x : red
		{XMFLOAT3(0.5f, -0.5f, -0.5f),  XMFLOAT3(0.80f, 0.12f, .23f)},	 //  4
		{XMFLOAT3(0.5f, -0.5f,  0.5f),  XMFLOAT3(0.80f, 0.12f, .23f)},	 //  5
		{XMFLOAT3(0.5f,  0.5f, -0.5f),  XMFLOAT3(0.80f, 0.12f, .23f)},	 //  6
		{XMFLOAT3(0.5f,  0.5f,  0.5f),  XMFLOAT3(0.80f, 0.12f, .23f)},	 //  7

		// -y : white
		{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},     //  8
		{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},     //  9
		{XMFLOAT3(0.5f, -0.5f, -0.5f),  XMFLOAT3(1.0f, 1.0f, 1.0f)},     // 10
		{XMFLOAT3(0.5f, -0.5f,  0.5f),  XMFLOAT3(1.0f, 1.0f, 1.0f)},     // 11

		// +y : yellow
		{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 0.84f, 0.0f)},    // 12
		{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 0.84f, 0.0f)},    // 13
		{XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 0.84f, 0.0f)},    // 14
		{XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 0.84f, 0.0f)},    // 15

		// -z : green
		{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.48f, 0.29f)},   // 16
		{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 0.48f, 0.29f)},   // 17
		{XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.48f, 0.29f)},   // 18
		{XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 0.48f, 0.29f)},   // 19
																				 
		// +z : blue
		{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.32f, 0.73f)},   // 20
		{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 0.32f, 0.73f)},   // 21
		{XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.32f, 0.73f)},   // 22
		{XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 0.32f, 0.73f)},   // 23
	};

	D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
	vertexBufferData.pSysMem = cubeVertices;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			m_vertexBuffer.put()
			)
		);

	// Load mesh indices. Each trio of indices represents
	// a triangle to be rendered on the screen.
	// For example: 0,2,1 means that the vertices with indexes
	// 0, 2 and 1 from the vertex buffer compose the 
	// first triangle of this mesh.
	static const unsigned short cubeIndices [] =
	{
		0,2,1, // -x
		1,2,3,

		4,5,6, // +x
		5,7,6,

		8,9,11, // -y
		8,11,10,

		12,14,15, // +y
		12,15,13,

		16,18,19, // -z
		16,19,17,

		20,21,23, // +z
		20,23,22,
	};

	m_indexCount = ARRAYSIZE(cubeIndices);

	D3D11_SUBRESOURCE_DATA indexBufferData = {0};
	indexBufferData.pSysMem = cubeIndices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			m_indexBuffer.put()));

	m_loadingComplete = true;
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader = nullptr;
	m_inputLayout = nullptr;
	m_pixelShader = nullptr;
	m_constantBuffer = nullptr;
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
}