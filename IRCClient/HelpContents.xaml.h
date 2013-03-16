//
// HelpContents.xaml.h
// Declaration of the HelpContents class
//

#pragma once

#include "Common\LayoutAwarePage.h" // Required by generated header
#include "HelpContents.g.h"

namespace IRCClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	public ref class HelpContents sealed
	{
	public:
		HelpContents();

	protected:
		virtual void LoadState(Platform::Object^ navigationParameter,
			Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
		virtual void SaveState(Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
	private:
		void gettingStartedLink_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void commandOverviewLink_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void creditsLink_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
