#ifdef NEWCODE
#define _USE_MATH_DEFINES
#endif

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

#ifdef NEWCODE
#include <math.h>
#endif

#include "NethackMain.h"
#include "Common\DirectXHelper.h"

#ifdef NEWCODE
#include "..\..\win\w8\w8_console.h"
#include "..\..\win\w8\w8_procs.h"
#include "..\..\win\w8\w8_window.h"
#endif

using namespace Nethack;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

extern Nethack::TextGrid g_textGrid;


// Loads and initializes application assets when the application is loaded.
NethackMain::NethackMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources)
{
    // Register to be notified if the Device is lost or recreated
    m_deviceResources->RegisterDeviceNotify(this);

    m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    // Initialize text grid state
    Nethack::Int2D & glyphPixelDimensions = DX::DeviceResources::s_deviceResources->GetGlyphPixelDimensions();
    Windows::Foundation::Size & outputSize = DX::DeviceResources::s_deviceResources->GetOutputSize();

    float xScale = outputSize.Width / (float)(80 * glyphPixelDimensions.m_x);
    float yScale = outputSize.Height / (float)(28 * glyphPixelDimensions.m_y);

    g_textGrid.SetDeviceResources();

    g_textGrid.SetScale((xScale < yScale) ? xScale : yScale);

    Nethack::Int2D gridOffset(0, 0);
    g_textGrid.SetGridOffset(gridOffset);

}

NethackMain::~NethackMain()
{
    // Deregister device notification
    m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void NethackMain::CreateWindowSizeDependentResources() 
{

#ifdef NEWCODE
    Window::s_windowListLock.AcquireShared();

    for (auto window : Window::s_windowList)
        window->CreateWindowSizeDependentResources();

    Window::s_windowListLock.ReleaseShared();
#endif

}

// Updates the application state once per frame.
void NethackMain::Update() 
{
    // Update scene objects.
    m_timer.Tick([&]()
    {
        m_fpsTextRenderer->Update(m_timer);

#ifdef NEWCODE
        Window::s_windowListLock.AcquireShared();

        for (auto window : Window::s_windowList)
            window->Update(m_timer);

        Window::s_windowListLock.ReleaseShared();
#endif

        

    });
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool NethackMain::Render() 
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return false;
    }

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Reset the viewport to target the whole screen.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    // Reset render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

    // Clear the back buffer and depth stencil view.
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_fpsTextRenderer->Render();

    g_textGrid.Render();

    return true;
}

// Notifies renderers that device resources need to be released.
void NethackMain::OnDeviceLost()
{
#ifdef NEWCODE
    Window::s_windowListLock.AcquireShared();

    for (auto window : Window::s_windowList)
        window->ReleaseDeviceDependentResources();

    Window::s_windowListLock.ReleaseShared();
#endif

    m_fpsTextRenderer->ReleaseDeviceDependentResources();
    g_textGrid.ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void NethackMain::OnDeviceRestored()
{
#ifdef NEWCODE
    Window::s_windowListLock.AcquireShared();

    for (auto window : Window::s_windowList)
        window->CreateDeviceDependentResources();

    Window::s_windowListLock.ReleaseShared();
#endif

    m_fpsTextRenderer->CreateDeviceDependentResources();
    g_textGrid.CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

#ifdef NEWCODE

//
// Keyboard Handlers
//

extern "C" { extern void wt_input(char c);  }

void NethackMain::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
}

void NethackMain::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
}

void NethackMain::OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args)
{
    w8_key_input((char)args->KeyCode);
}


//
// Gesture Event Handlers
//

void NethackMain::OnPointerPressed(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args)
{
#if 0
    unsigned int pointerId = args->CurrentPoint->PointerId;

    Windows::UI::Input::PointerPoint^ pointerPoint = Windows::UI::Input::PointerPoint::GetCurrentPoint(pointerId);

    Float2D screenPosition(pointerPoint->RawPosition.X, pointerPoint->RawPosition.Y);

    for (auto textGrid : m_textGrids)
    {
        if (textGrid->HitTest(screenPosition))
        {
            m_hitTextGrid = textGrid;
            gestureRecognizer->ProcessDownEvent(pointerPoint);
            return;
        }
    }
#else
    unsigned int pointerId = args->CurrentPoint->PointerId;

    Windows::UI::Input::PointerPoint^ pointerPoint = Windows::UI::Input::PointerPoint::GetCurrentPoint(pointerId);

    gestureRecognizer->ProcessDownEvent(pointerPoint);
#endif

}

void NethackMain::OnPointerMoved(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args)
{
    unsigned int pointerId = args->CurrentPoint->PointerId;

    Windows::Foundation::Collections::IVector<Windows::UI::Input::PointerPoint^>^ pointerPoints =
        Windows::UI::Input::PointerPoint::GetIntermediatePoints(pointerId);

    gestureRecognizer->ProcessMoveEvents(pointerPoints);

}

void NethackMain::OnPointerReleased(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args)
{
    unsigned int pointerId = args->CurrentPoint->PointerId;

    Windows::UI::Input::PointerPoint^ pointerPoint = Windows::UI::Input::PointerPoint::GetCurrentPoint(
        pointerId);

    gestureRecognizer->ProcessUpEvent(pointerPoint);

    //    m_hitTextGrid = nullptr;
}

void NethackMain::OnPointerWheelChanged(Windows::UI::Input::GestureRecognizer^ gestureRecognizer, Windows::UI::Core::PointerEventArgs^ args)
{
    unsigned int pointerId = args->CurrentPoint->PointerId;

    // Create transformed PointerPoint relative to parent object
    Windows::UI::Input::PointerPoint^ pointerPoint = Windows::UI::Input::PointerPoint::GetCurrentPoint(pointerId);

    // Set mouse wheel parameters
    //    Windows::UI::Input::MouseWheelParameters^ params = m_gestureRecognizer->MouseWheelParameters;
    //    Windows::Foundation::Size viewSize = ViewSize();
    //    params->CharTranslation = Windows::Foundation::Point(0.05f * viewSize.Width, 0.05f * viewSize.Height);
    //    params->PageTranslation = Windows::Foundation::Point(viewSize.Width, viewSize.Height);
    //    params->DeltaScale = 1.5f;
    //    params->DeltaRotationAngle = 22.5f;

    // Assign wheel pointer to background.
    Windows::System::VirtualKeyModifiers vkmod = args->KeyModifiers;
    gestureRecognizer->ProcessMouseWheelEvent(
        pointerPoint,
        (int)(vkmod & Windows::System::VirtualKeyModifiers::Shift) != 0,
        (int)(vkmod & Windows::System::VirtualKeyModifiers::Control) != 0);
}

void NethackMain::OnHolding(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::HoldingEventArgs^ args)
{

}

void NethackMain::OnRightTapped(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::RightTappedEventArgs^ args)
{

}

void NethackMain::OnTapped(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::TappedEventArgs^ args)
{
    int x = DX::ConvertDipsToPixels(args->Position.X, m_deviceResources->GetDpi());
    int y = DX::ConvertDipsToPixels(args->Position.Y, m_deviceResources->GetDpi());

    Float2D screenPosition(x, y);

    w8_mouse_input(screenPosition);

}

void NethackMain::OnManipulationStarted(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationStartedEventArgs^ args)
{
    m_flickDirection = Direction::None;
}


NethackMain::Direction NethackMain::AngleToDirection(float inAngle)
{
    assert(inAngle <= 180.0 && inAngle >= -180.0);

    inAngle += 180.0;   // 0 - 360 -> 0:left, 90:down, 180:right, 270: up, 360 left
    inAngle /= 45.0;    // 0 - 8 -> 0:left, 2:down, 4:right, 6:up, 8: left 

    int direction = (int)round(inAngle);

    if (direction == 8) direction = 0;

    return (Direction)direction;

}


void NethackMain::OnManipulationUpdated(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationUpdatedEventArgs^ args)
{
    float dx = args->Cumulative.Translation.X;
    float dy = args->Cumulative.Translation.Y;
    float angle = (360.0f * atan2f(dy, dx)) / (2.0f * (float)M_PI);

    m_flickDirection = AngleToDirection(angle);
}

void NethackMain::OnManipulationInertiaStarting(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationInertiaStartingEventArgs^ args)
{

}

void NethackMain::OnManipulationCompleted(Windows::UI::Input::GestureRecognizer^ sender, Windows::UI::Input::ManipulationCompletedEventArgs^ args)
{
    if (m_flickDirection != Direction::None)
    {
        static char directionToKey[] = { '4', '7', '8', '9', '6', '3', '2', '1' };
        char key = directionToKey[(int)m_flickDirection];
        w8_key_input(key);
    }
}
#endif
