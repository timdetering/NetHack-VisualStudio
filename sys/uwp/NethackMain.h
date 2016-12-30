#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Content\SampleFpsTextRenderer.h"
#include "common\TextGrid.h"

#ifdef NEWCODE
#include <list>
#endif

// Renders Direct2D and 3D content on the screen.
namespace Nethack
{
    class NethackMain : public DX::IDeviceNotify
    {
    public:
        NethackMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        ~NethackMain();
        void CreateWindowSizeDependentResources();
        void Update();
        bool Render();

        // IDeviceNotify
        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();

#ifdef NEWCODE
        void Attach(_In_ Windows::UI::Input::GestureRecognizer^ gestureRecognizer);

        // Gesture event handlers
        void OnPointerPressed(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerMoved(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerReleased(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args);
        void OnPointerWheelChanged(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args);

        void OnHolding(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::HoldingEventArgs^ args);
        void OnTapped(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::TappedEventArgs^ args);
        void OnRightTapped(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::RightTappedEventArgs^ args);
        void OnManipulationStarted(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationStartedEventArgs^ args);
        void OnManipulationUpdated(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationUpdatedEventArgs^ args);
        void OnManipulationInertiaStarting(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationInertiaStartingEventArgs^ args);
        void OnManipulationCompleted(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationCompletedEventArgs^ args);
#endif

        // Keyboard
        void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);

    private:

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // TODO: Replace with your own content renderers.
//		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
        std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

        // Rendering loop timer.
        DX::StepTimer m_timer;

#ifdef NEWCODE
        Direction m_flickDirection;

        enum class Direction { Left, LeftUp, Up, RightUp, Right, RightDown, Down, LeftDown, None };

        Direction AngleToDirection(float inAngle);
#endif

    };
}
