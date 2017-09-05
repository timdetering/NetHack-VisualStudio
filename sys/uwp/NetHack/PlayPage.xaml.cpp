//
// PlayPage.xaml.cpp
// Implementation of the PlayPage class
//

#include "pch.h"
#include "PlayPage.xaml.h"
#include "MainPage.xaml.h"

using namespace NetHack;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Interop;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

PlayPage::PlayPage()
{
	InitializeComponent();
}

void NetHack::PlayPage::ScrollViewer_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::I)
    {
        if (InventoryScrollViewer->Visibility == Windows::UI::Xaml::Visibility::Collapsed) {
            InventoryScrollViewer->Visibility = Windows::UI::Xaml::Visibility::Visible;
            MapScrollViewer->Width = LayoutMiddle->ActualWidth - InventoryScrollViewer->ActualWidth;
        }
        else {
            InventoryScrollViewer->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            MapScrollViewer->Width = LayoutMiddle->ActualWidth;
        }

    }

    if (e->Key == Windows::System::VirtualKey::Q)
    {
        this->Frame->Navigate(TypeName(MainPage::typeid));
    }

    if (e->Key == Windows::System::VirtualKey::X)
    {
        static boolean toggle = 0;
        String ^ text;
        if (toggle) text = "AAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHH";
        else text = "00000000001111111111222222222233333333334444444444555555555566666666667777777777";
        TestText->Text = "test";
        for (unsigned int i = 0; i < LayoutMap->Children->Size; i++)
        {
            TextBlock ^ textBlock = (TextBlock ^)LayoutMap->Children->GetAt(i);
            textBlock->Text = text;
        }
        toggle = !toggle;
    }
}


void NetHack::PlayPage::LayoutMiddle_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
    if (InventoryScrollViewer->Visibility == Windows::UI::Xaml::Visibility::Collapsed) {
        MapScrollViewer->Width = LayoutMiddle->ActualWidth;
    }
    else {
        MapScrollViewer->Width = LayoutMiddle->ActualWidth - InventoryScrollViewer->ActualWidth;
    }
}
