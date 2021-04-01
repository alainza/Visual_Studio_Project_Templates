#pragma once

#include <string>
#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"

namespace winrt::$projectname$::implementation
{
	// Renders the current FPS value in the bottom right corner of the screen using Direct2D and DirectWrite.
	class SampleFpsTextRenderer
	{
	public:
		SampleFpsTextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Resources related to text rendering.
		std::wstring                                    m_text;
		DWRITE_TEXT_METRICS	                            m_textMetrics;
		winrt::com_ptr<ID2D1SolidColorBrush>    m_whiteBrush;
		winrt::com_ptr<ID2D1DrawingStateBlock1> m_stateBlock;
		winrt::com_ptr<IDWriteTextLayout3>      m_textLayout;
		winrt::com_ptr<IDWriteTextFormat2>      m_textFormat;
	};
}