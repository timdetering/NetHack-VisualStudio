//
// PlayPage.xaml.h
// Declaration of the PlayPage class
//

#pragma once

#include "PlayPage.g.h"

namespace NetHack
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PlayPage sealed
	{
	public:
		PlayPage();
    private:
        void Quit_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
        void DoQuit(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };
}
