#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::$projectname$::implementation;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::System::Threading;

MainPage::MainPage()
{
    InitializeComponent();

    auto window = Window::Current().CoreWindow();

    m_visibilityChangedRevoker = window.VisibilityChanged(
		winrt::auto_revoke, { this, &MainPage::OnVisibilityChanged });

	auto currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_dpiChangedRevoker = currentDisplayInformation.DpiChanged(
		winrt::auto_revoke, { this, &MainPage::OnDpiChanged });

	m_orientationChangedRevoker = currentDisplayInformation.OrientationChanged(
		winrt::auto_revoke, { this, &MainPage::OnOrientationChanged });

	m_displayContentsInvalidated_revoker = DisplayInformation::DisplayContentsInvalidated(
		winrt::auto_revoke, { this, &MainPage::OnDisplayContentsInvalidated });

	m_compositionScaleChanged_revoker = swapChainPanel().CompositionScaleChanged(
		winrt::auto_revoke, { this, &MainPage::OnCompositionScaleChanged });

	m_sizeChanged_revoker = swapChainPanel().SizeChanged(
		winrt::auto_revoke, { this,  &MainPage::OnSwapChainPanelSizeChanged });

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
	m_deviceResources->SetSwapChainPanel(swapChainPanel());

	// Register our SwapChainPanel to get independent input pointer events
	auto workItemHandler = [this](IAsyncAction const&)
	{
		// The CoreIndependentInputSource will raise pointer events for the specified device types on whichever thread it's created on.
		m_coreInput = swapChainPanel().CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen
		);

		// Register for pointer events, which will be raised on the background thread.
		m_pointerPressed_revoker = m_coreInput.PointerPressed(
			winrt::auto_revoke, { this,  &MainPage::OnPointerPressedZ });
		m_pointerMoved_revoker = m_coreInput.PointerMoved(
			winrt::auto_revoke, { this,  &MainPage::OnPointerMovedZ });
		m_pointerReleased_revoker = m_coreInput.PointerReleased(
			winrt::auto_revoke, { this,  &MainPage::OnPointerReleasedZ });

		// Begin processing input messages as they're delivered.
		m_coreInput.Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	};

	// Run task on a dedicated high priority background thread.
	m_inputLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

	m_main = std::unique_ptr<$projectname$Main>(new $projectname$Main(m_deviceResources));
	m_main->StartRenderLoop();
	OutputDebugString(L"Leaving MainPage constructon\n");
}

MainPage::~MainPage()
{
	// Stop rendering and processing events on destruction.
	m_main->StopRenderLoop();
	m_coreInput.Dispatcher().StopProcessEvents();
}

void MainPage::OnVisibilityChanged(
	winrt::Windows::UI::Core::CoreWindow const& /*sender*/,
	winrt::Windows::UI::Core::VisibilityChangedEventArgs const& args)
{
	m_windowVisible = args.Visible();
	if (m_windowVisible)
	{
		m_main->StartRenderLoop();
	}
	else
	{
		m_main->StopRenderLoop();
	}
}

void MainPage::OnDpiChanged(
	winrt::Windows::Graphics::Display::DisplayInformation const& sender,
	winrt::Windows::Foundation::IInspectable const& /*args*/)
{
	std::lock_guard<std::mutex> lock(m_main->CriticalSection());
	// Note: The value for LogicalDpi retrieved here may not match the effective DPI of the app
	// if it is being scaled for high resolution devices. Once the DPI is set on DeviceResources,
	// you should always retrieve it using the GetDpi method.
	// See DeviceResources.cpp for more details.
	m_deviceResources->SetDpi(sender.LogicalDpi());
	m_main->CreateWindowSizeDependentResources();
}

void MainPage::OnOrientationChanged(
	winrt::Windows::Graphics::Display::DisplayInformation const& sender,
	winrt::Windows::Foundation::IInspectable const& /*args*/)
{
	std::lock_guard<std::mutex> lock(m_main->CriticalSection());
	m_deviceResources->SetCurrentOrientation(sender.CurrentOrientation());
	m_main->CreateWindowSizeDependentResources();
}

void MainPage::OnDisplayContentsInvalidated(
	winrt::Windows::Graphics::Display::DisplayInformation const& /*sender*/,
	winrt::Windows::Foundation::IInspectable const& /*args*/)
{
	std::lock_guard<std::mutex> lock(m_main->CriticalSection());
	m_deviceResources->ValidateDevice();
}

// Called when the app bar button is clicked.
void MainPage::AppBarButton_Click(
	winrt::Windows::Foundation::IInspectable const& /*sender*/,
	winrt::Windows::UI::Xaml::RoutedEventArgs const& /*e*/)
{
	// Use the app bar if it is appropriate for your app. Design the app bar, 
	// then fill in event handlers (like this one).
}

void MainPage::OnPointerPressedZ(
	winrt::Windows::Foundation::IInspectable const& /*sender*/,
	winrt::Windows::UI::Core::PointerEventArgs const& /*e*/)
{
	// When the pointer is pressed begin tracking the pointer movement.
	m_main->StartTracking();
}

void MainPage::OnPointerMovedZ(
	winrt::Windows::Foundation::IInspectable const& /*sender*/,
	winrt::Windows::UI::Core::PointerEventArgs const& e)
{
	// Update the pointer tracking code.
	if (m_main->IsTracking())
	{
		m_main->TrackingUpdate(e.CurrentPoint().Position().X);
	}
}

void MainPage::OnPointerReleasedZ(
	winrt::Windows::Foundation::IInspectable const& /*sender*/,
	winrt::Windows::UI::Core::PointerEventArgs const& /*e*/)
{
	// Stop tracking pointer movement when the pointer is released.
	m_main->StopTracking();
}

void MainPage::OnCompositionScaleChanged(
	winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& sender,
	winrt::Windows::Foundation::IInspectable const& /*args*/)
{
	std::lock_guard<std::mutex> lock(m_main->CriticalSection());
	m_deviceResources->SetCompositionScale(sender.CompositionScaleX(), sender.CompositionScaleY());
	m_main->CreateWindowSizeDependentResources();
}

void MainPage::OnSwapChainPanelSizeChanged(
	winrt::Windows::Foundation::IInspectable const& /*sender*/,
	winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e)
{
	std::lock_guard<std::mutex> lock(m_main->CriticalSection());
	m_deviceResources->SetLogicalSize(e.NewSize());
	m_main->CreateWindowSizeDependentResources();
}
