//
// EditServer.xaml.h
// Declaration of the EditServer class
//

#pragma once

#include "Common\LayoutAwarePage.h" // Required by generated header
#include "EditServer.g.h"

namespace IRCClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	public ref class EditServer sealed
	{
	public:
		EditServer();

	protected:
		virtual void LoadState(Platform::Object^ navigationParameter,
		Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
		virtual void SaveState(Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;

	private:
		void serverNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverHostNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverPortTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverUsernameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverPasswordTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverUserHostnameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverUserRealnameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void pageRoot_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
