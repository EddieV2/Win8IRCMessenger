// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include "Common\LayoutAwarePage.h" // Required by generated header
#include "MainPage.g.h"

namespace IRCClient
{
	// Forward references.
	ref class EventHookedIRCServer;
	ref class EventHookedIRCChannel;

	// Some tweak settings.
	#define MAX_MESSAGE_TO_SHOW_AT_ONCE 75
	
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		Platform::Collections::Vector<EventHookedIRCServer^>^	m_eventHookedIRCServers;
		Platform::Collections::Vector<EventHookedIRCChannel^>^	m_eventHookedIRCChannels;
		unsigned int											m_refreshWindowCounter;


		void HookServerEvents();
		void UnhookServerEvents();

		void ServerConnectionEvent				(LibIRC::IRCServer^ server);
		void ServerChannelJoinEvent				(LibIRC::IRCServer^ server, LibIRC::IRCChannel^ channel, LibIRC::IRCUser^ user);
		void ServerChannelEvent					(LibIRC::IRCServer^ server, LibIRC::IRCChannel^ channel, LibIRC::IRCUser^ user);
		void ServerUserNicknameChangedEvent		(LibIRC::IRCServer^ server, LibIRC::IRCUser^ user);
		void ServerUserChannelModeChangedEvent	(LibIRC::IRCServer^ server, LibIRC::IRCChannel^ channel, LibIRC::IRCUser^ user);
		void ServerMessageRecievedEvent			(LibIRC::IRCServer^ server, LibIRC::IRCMessage^ message);
		void RefreshServerWindow				(bool refreshUserList);
		void RefreshServerList					();
		void RefreshRoomList					();
		void RefreshMessageList					(bool refreshUserList);
		void SendMessage						();

		void RunUIAfterDelay			(unsigned int delay, Windows::UI::Core::DispatchedHandler^ handler);

		void SelectAServerButton_Click	(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_Loaded_1				(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_Unloaded_1			(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		
		void OnServerTapped				(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
		void OnChannelTapped			(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);

	public:
		MainPage();
	private:
		void JoinButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void LeaveButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void DisconnectButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void SettingsButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void HelpButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void sayTextBox_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
		void sendButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_SizeChanged_1(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void userScrollViewer_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Input::RightTappedRoutedEventArgs^ e);
		void userScrollViewer_ItemClickTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
	};

	/// <summary>
	///	Stores information on an irc server that has its events currently hooked.
	/// </summary>	
	public ref class EventHookedIRCServer sealed
	{
	private:
		LibIRC::IRCServer^							 m_server;		
		Windows::Foundation::EventRegistrationToken  m_serverConnectionSuccessEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverConnectionFailureEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverConnectionDisconnectEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverJoinChannelEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverLeaveChannelEventCookie;
		Windows::Foundation::EventRegistrationToken  m_serverMessageRecievedEventCookie;
		Windows::Foundation::EventRegistrationToken	 m_serverUserChannelModeChangedEventCookie;
		Windows::Foundation::EventRegistrationToken	 m_serverUserNicknameChangedEventCookie;

		Windows::UI::Xaml::Controls::Grid^			m_gridControl;
		Windows::UI::Xaml::Controls::TextBlock^		m_nameTextBlockControl;
		Windows::UI::Xaml::Controls::TextBlock^		m_statusTextBlockControl;
		Windows::UI::Xaml::Controls::Button^		m_closeButtonControl;

	public:
		property LibIRC::IRCServer^ Server
		{
			LibIRC::IRCServer^ get()			{ return m_server; }
			void set(LibIRC::IRCServer^ value)	{ m_server = value; }
		}

		property Windows::Foundation::EventRegistrationToken ServerConnectSuccessEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverConnectionSuccessEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverConnectionSuccessEventCookie = value; }
		}

		property Windows::Foundation::EventRegistrationToken ServerConnectFailureEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverConnectionFailureEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverConnectionFailureEventCookie = value; }
		}

		property Windows::Foundation::EventRegistrationToken ServerConnectDisconnectEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverConnectionDisconnectEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverConnectionDisconnectEventCookie = value; }
		}

		property Windows::Foundation::EventRegistrationToken ServerJoinChannelEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverJoinChannelEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverJoinChannelEventCookie = value; }
		}

		property Windows::Foundation::EventRegistrationToken ServerLeaveChannelEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverLeaveChannelEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverLeaveChannelEventCookie = value; }
		}
		
		property Windows::Foundation::EventRegistrationToken ServerMessageRecievedEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverMessageRecievedEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverMessageRecievedEventCookie = value; }
		}

		property Windows::Foundation::EventRegistrationToken ServerUserChannelModeChangedEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverUserChannelModeChangedEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverUserChannelModeChangedEventCookie = value; }
		}
		
		property Windows::Foundation::EventRegistrationToken ServerUserNicknameChangedEventCookie
		{
			Windows::Foundation::EventRegistrationToken get()			{ return m_serverUserNicknameChangedEventCookie; }
			void set(Windows::Foundation::EventRegistrationToken value)	{ m_serverUserNicknameChangedEventCookie = value; }
		}

		property Windows::UI::Xaml::Controls::Grid^	GridControl
		{
			Windows::UI::Xaml::Controls::Grid^ get()			{ return m_gridControl; }
			void set(Windows::UI::Xaml::Controls::Grid^ value)	{ m_gridControl = value; }
		}

		property Windows::UI::Xaml::Controls::TextBlock^ NameTextBlockControl
		{
			Windows::UI::Xaml::Controls::TextBlock^ get()			{ return m_nameTextBlockControl; }
			void set(Windows::UI::Xaml::Controls::TextBlock^ value)	{ m_nameTextBlockControl = value; }
		}

		property Windows::UI::Xaml::Controls::TextBlock^ StatusTextBlockControl
		{
			Windows::UI::Xaml::Controls::TextBlock^ get()			{ return m_statusTextBlockControl; }
			void set(Windows::UI::Xaml::Controls::TextBlock^ value)	{ m_statusTextBlockControl = value; }
		}

		property Windows::UI::Xaml::Controls::Button^ CloseButtonControl
		{
			Windows::UI::Xaml::Controls::Button^ get()				{ return m_closeButtonControl; }
			void set(Windows::UI::Xaml::Controls::Button^ value)	{ m_closeButtonControl = value; }
		}

	};

	/// <summary>
	///	Stores information on an irc channel that has its events currently hooked.
	/// </summary>	
	public ref class EventHookedIRCChannel sealed
	{
	private:
		LibIRC::IRCServer^						m_server;		
		LibIRC::IRCChannel^						m_channel;		

		Windows::UI::Xaml::Controls::Grid^			m_gridControl;
		Windows::UI::Xaml::Controls::TextBlock^		m_nameTextBlockControl;
		Windows::UI::Xaml::Controls::TextBlock^		m_statusTextBlockControl;
		Windows::UI::Xaml::Controls::Button^		m_closeButtonControl;


	public:
		property LibIRC::IRCServer^ Server
		{
			LibIRC::IRCServer^ get()			{ return m_server; }
			void set(LibIRC::IRCServer^ value)	{ m_server = value; }
		}

		property LibIRC::IRCChannel^ Channel
		{
			LibIRC::IRCChannel^ get()			{ return m_channel; }
			void set(LibIRC::IRCChannel^ value)	{ m_channel = value; }
		}

		property Windows::UI::Xaml::Controls::Grid^	GridControl
		{
			Windows::UI::Xaml::Controls::Grid^ get()			{ return m_gridControl; }
			void set(Windows::UI::Xaml::Controls::Grid^ value)	{ m_gridControl = value; }
		}

		property Windows::UI::Xaml::Controls::TextBlock^ NameTextBlockControl
		{
			Windows::UI::Xaml::Controls::TextBlock^ get()			{ return m_nameTextBlockControl; }
			void set(Windows::UI::Xaml::Controls::TextBlock^ value)	{ m_nameTextBlockControl = value; }
		}

		property Windows::UI::Xaml::Controls::TextBlock^ StatusTextBlockControl
		{
			Windows::UI::Xaml::Controls::TextBlock^ get()			{ return m_statusTextBlockControl; }
			void set(Windows::UI::Xaml::Controls::TextBlock^ value)	{ m_statusTextBlockControl = value; }
		}

		property Windows::UI::Xaml::Controls::Button^ CloseButtonControl
		{
			Windows::UI::Xaml::Controls::Button^ get()				{ return m_closeButtonControl; }
			void set(Windows::UI::Xaml::Controls::Button^ value)	{ m_closeButtonControl = value; }
		}

	};
}
