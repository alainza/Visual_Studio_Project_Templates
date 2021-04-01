#pragma once

#include "MainPage.g.h"
#include "Common\DeviceResources.h"
#include "$projectname$Main.h"

namespace winrt::$projectname$::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        ~MainPage();

    private:
        // Window event handlers.
        void OnVisibilityChanged(
            winrt::Windows::UI::Core::CoreWindow const& sender,
            winrt::Windows::UI::Core::VisibilityChangedEventArgs const& args);

        // DisplayInformation event handlers.
        void OnDpiChanged(
            winrt::Windows::Graphics::Display::DisplayInformation const& sender,
            winrt::Windows::Foundation::IInspectable const& args);

        void OnOrientationChanged(
            winrt::Windows::Graphics::Display::DisplayInformation const& sender,
            winrt::Windows::Foundation::IInspectable const& args);

        void OnDisplayContentsInvalidated(
            winrt::Windows::Graphics::Display::DisplayInformation const& sender,
            winrt::Windows::Foundation::IInspectable const& args);

        // Other event handlers.
    public:       
        void AppBarButton_Click(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
    private:

        void OnCompositionScaleChanged(
            winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& sender,
            winrt::Windows::Foundation::IInspectable const& args);
        void OnSwapChainPanelSizeChanged(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Xaml::SizeChangedEventArgs const& e);

        // Track our independent input on a background worker thread.
        winrt::Windows::Foundation::IAsyncAction m_inputLoopWorker{ nullptr };
        winrt::Windows::UI::Core::CoreIndependentInputSource m_coreInput{ nullptr };

        // Independent input handling functions.
        void OnPointerPressedZ(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Core::PointerEventArgs const& e);
        void OnPointerMovedZ(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Core::PointerEventArgs const& e);
        void OnPointerReleasedZ(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Windows::UI::Core::PointerEventArgs const& e);

        // Resources used to render the DirectX content in the XAML page background.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;
        std::unique_ptr<$projectname$Main> m_main;
        bool m_windowVisible{ true };

        // Event revokers
        winrt::Windows::UI::Core::CoreWindow::VisibilityChanged_revoker m_visibilityChangedRevoker;
        winrt::Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker m_dpiChangedRevoker;
        winrt::Windows::Graphics::Display::DisplayInformation::OrientationChanged_revoker m_orientationChangedRevoker;
        winrt::Windows::Graphics::Display::DisplayInformation::DisplayContentsInvalidated_revoker m_displayContentsInvalidated_revoker;
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel::CompositionScaleChanged_revoker m_compositionScaleChanged_revoker;
        winrt::Windows::UI::Xaml::Controls::SwapChainPanel::SizeChanged_revoker m_sizeChanged_revoker;
        winrt::Windows::UI::Core::CoreIndependentInputSource::PointerPressed_revoker m_pointerPressed_revoker;
        winrt::Windows::UI::Core::CoreIndependentInputSource::PointerMoved_revoker m_pointerMoved_revoker;
        winrt::Windows::UI::Core::CoreIndependentInputSource::PointerReleased_revoker m_pointerReleased_revoker;;

    };
}

namespace winrt::$projectname$::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
