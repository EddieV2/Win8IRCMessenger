// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include "App.g.h"

namespace IRCClient
{
	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	private:		
		static App^ m_instance;

		LibIRC::IRCController^	m_controller;
		LibIRC::IRCServer^		m_currentServer;
		LibIRC::IRCChannel^		m_currentChannel;
		LibIRC::IRCServer^		m_currentEditServer;
		
		bool					m_recievedNetworkOptIn;

		bool													m_settingsEventRegistered;
		Windows::Foundation::EventRegistrationToken				m_commandsRequestedToken;

		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnNetworkStatusChanged(Platform::Object^ sender);

		void OnSettingsPaneCommandRequested(Windows::UI::ApplicationSettings::SettingsPane^ sender, Windows::UI::ApplicationSettings::SettingsPaneCommandsRequestedEventArgs^ args);
		void DoSettingsPaneOperation(Windows::UI::Popups::IUICommand^ command);

		void LoadDefaultServers	();

	public:
		static property App^ Instance
		{
			App^ get() { return m_instance; }
		}

		property LibIRC::IRCController^ IRCController
		{
			LibIRC::IRCController^ get() { return m_controller; }
		}

		property LibIRC::IRCServer^ CurrentServer
		{
			LibIRC::IRCServer^ get() { return m_currentServer; }
			void set(LibIRC::IRCServer^ value) { m_currentServer = value; }
		}

		property LibIRC::IRCServer^ CurrentEditServer
		{
			LibIRC::IRCServer^ get() { return m_currentEditServer; }
			void set(LibIRC::IRCServer^ value) { m_currentEditServer = value; }
		}

		property LibIRC::IRCChannel^ CurrentChannel
		{
			LibIRC::IRCChannel^ get() { return m_currentChannel; }
			void set(LibIRC::IRCChannel^ value) { m_currentChannel = value; }
		}

		App();
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;		
		virtual void OnWindowCreated(Windows::UI::Xaml::WindowCreatedEventArgs^ args) override;

		void SaveState					();
		void LoadState					();

		bool CheckForMeteredNetwork		();
	};
}
