//
// SelectServer.xaml.h
// Declaration of the SelectServer class
//

#pragma once

#include "Common\LayoutAwarePage.h" // Required by generated header
#include "SelectServer.g.h"

namespace IRCClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	public ref class SelectServer sealed
	{
	public:
		SelectServer();
		
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

		void addServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void serverListBox_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void deleteServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void editServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void connectServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void pageRoot_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void pageRoot_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

	protected:
		virtual void LoadState(Platform::Object^ navigationParameter,
		Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
		virtual void SaveState(Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;

	};
}
