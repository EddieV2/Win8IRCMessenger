//
// SelectServer.xaml.h
// Declaration of the SelectServer class
//

#pragma once

#include "Common\LayoutAwarePage.h" // Required by generated header
#include "ConfigureServer.g.h"

namespace IRCClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	public ref class ConfigureServer sealed
	{
	public:
		ConfigureServer();

	protected:
		virtual void LoadState(Platform::Object^ navigationParameter,
			Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
		virtual void SaveState(Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;

	private:

		Windows::Foundation::EventRegistrationToken  m_serverConnectionFailureEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverConnectionSuccessEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverDisconnectEventCookie;
		LibIRC::IRCServer^							 m_registeredServer;

		void OnServerConnectionFailure(LibIRC::IRCServer^ server);
		void OnServerConnectionSuccess(LibIRC::IRCServer^ server);
		void OnServerDisconnect(LibIRC::IRCServer^ server);
		void RegisterServerEvents(LibIRC::IRCServer^ server);
		void UnregisterServerEvents();
		void RefreshServerInfo(bool error, bool byDisconnect);

		void pageRoot_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void serverListBox_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void serverNameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverHostNameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverPortTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverUsernameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverPasswordTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverUserHostnameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void serverUserRealnameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
		void addServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void deleteServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void serverNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void serverHostNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void serverPortTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void serverUsernameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void connectToServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	private:
		void pageRoot_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
