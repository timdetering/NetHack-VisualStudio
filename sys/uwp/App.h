#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>

#include "Common\DeviceResources.h"
#include "NethackMain.h"

namespace Nethack
{
    // Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
    ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
    {
    public:
        App();

        // IFrameworkView Methods.
        virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
        virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
        virtual void Load(Platform::String^ entryPoint);
        virtual void Run();
        virtual void Uninitialize();

    protected:
        // Application lifecycle event handlers.
        void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
        void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
        void OnResuming(Platform::Object^ sender, Platform::Object^ args);

        // Window event handlers.
        void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
        void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
        void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

        // DisplayInformation event handlers.
        void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

#ifdef NEWCODE
        // Pointer event handlers
        void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerWheelChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
#endif

        // Keyboard event handlers
        void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);

#ifdef NEWCODE
        // gesture support
        void Attach(_In_ Windows::UI::Input::GestureRecognizer^ gestureRecognizer);

#endif

        // Nethack main loop
        void RunNethackMainLoop(void);

    private:
        bool m_exit;

        std::shared_ptr<DX::DeviceResources> m_deviceResources;
        std::unique_ptr<NethackMain> m_main;
        bool m_windowClosed;
        bool m_windowVisible;

        // worker thread which runs nethack main loop
        Windows::Foundation::IAsyncAction^     m_nethackWorker;

#ifdef NEWCODE

        // gesture support
        Windows::UI::Input::GestureRecognizer^ m_gestureRecognizer;

        Windows::Foundation::EventRegistrationToken m_tokenHolding;
        Windows::Foundation::EventRegistrationToken m_tokenTapped;
        Windows::Foundation::EventRegistrationToken m_tokenRightTapped;
        Windows::Foundation::EventRegistrationToken m_tokenManipulationStarted;
        Windows::Foundation::EventRegistrationToken m_tokenManipulationUpdated;
        Windows::Foundation::EventRegistrationToken m_tokenManipulationInertiaStarting;
        Windows::Foundation::EventRegistrationToken m_tokenManipulationCompleted;

        void OnHolding(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::HoldingEventArgs^ args);
        void OnTapped(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::TappedEventArgs^ args);
        void OnRightTapped(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::RightTappedEventArgs^ args);
        void OnManipulationStarted(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationStartedEventArgs^ args);
        void OnManipulationUpdated(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationUpdatedEventArgs^ args);
        void OnManipulationInertiaStarting(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationInertiaStartingEventArgs^ args);
        void OnManipulationCompleted(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationCompletedEventArgs^ args);
#endif

    };
}

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
