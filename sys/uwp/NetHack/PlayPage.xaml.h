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
        void ScrollViewer_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
        void LayoutMiddle_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
    };
}
