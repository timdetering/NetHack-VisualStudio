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



void NetHack::PlayPage::Quit_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{

}


void NetHack::PlayPage::DoQuit(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Frame->Navigate(TypeName(MainPage::typeid));
}
