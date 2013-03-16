//
// SelectServer.xaml.cpp
// Implementation of the SelectServer class
//

#include "pch.h"
#include "ConfigureServer.xaml.h"

using namespace IRCClient;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Basic Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234237

SelectServer::SelectServer()
{
	InitializeComponent();

	m_registeredServer = nullptr;
}

/// <summary>
/// Populates the page with content passed during navigation.  Any saved state is also
/// provided when recreating a page from a prior session.
/// </summary>
/// <param name="navigationParameter">The parameter value passed to
/// <see cref="Frame::Navigate(Type, Object)"/> when this page was initially requested.
/// </param>
/// <param name="pageState">A map of state preserved by this page during an earlier
/// session.  This will be null the first time a page is visited.</param>
void SelectServer::LoadState(Object^ navigationParameter, IMap<String^, Object^>^ pageState)
{
	(void) navigationParameter;	// Unused parameter
	(void) pageState;	// Unused parameter
}

/// <summary>
/// Preserves state associated with this page in case the application is suspended or the
/// page is discarded from the navigation cache.  Values must conform to the serialization
/// requirements of <see cref="SuspensionManager::SessionState"/>.
/// </summary>
/// <param name="pageState">An empty map to be populated with serializable state.</param>
void SelectServer::SaveState(IMap<String^, Object^>^ pageState)
{
	(void) pageState;	// Unused parameter
}

/// <summary>
/// Registers the given server as the one we will be recieving disconnect/connect messages from.
/// </summary>
void IRCClient::SelectServer::RegisterServerEvents(LibIRC::IRCServer^ server)
{
	if (m_registeredServer != nullptr)
	{
		UnregisterServerEvents();
	}

	m_serverConnectionFailureEventCookie = (server->OnConnectFailure += ref new LibIRC::IRCConnectFailureDelegate(this, &IRCClient::SelectServer::OnServerConnectionFailure));
	m_serverConnectionSuccessEventCookie = (server->OnConnectSuccess += ref new LibIRC::IRCConnectSuccessDelegate(this, &IRCClient::SelectServer::OnServerConnectionSuccess));
	m_serverDisconnectEventCookie	     = (server->OnDisconnect += ref new LibIRC::IRCDisconnectDelegate(this, &IRCClient::SelectServer::OnServerDisconnect));

	m_registeredServer = server;
}

/// <summary>
/// Unregisters current server events.
/// </summary>
void IRCClient::SelectServer::UnregisterServerEvents()
{
	if (m_registeredServer == nullptr)
	{
		return;
	}

	m_registeredServer->OnConnectFailure	-= m_serverConnectionFailureEventCookie;
	m_registeredServer->OnConnectSuccess	-= m_serverConnectionSuccessEventCookie;
	m_registeredServer->OnDisconnect		-= m_serverDisconnectEventCookie;
	m_registeredServer = nullptr;
}

/// <summary>
/// Invoked when the page loads.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::SelectServer::pageRoot_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	IVector<LibIRC::IRCServer^>^ servers = App::Instance->IRCController->GetServerList();

	for (unsigned int i = 0; i < servers->Size; i++)
	{
		LibIRC::IRCServer^ server = servers->GetAt(i);

		ListBoxItem^ item = ref new ListBoxItem();
		item->Content = server->Name;
		item->IsSelected = (i == 0);
		item->Tag = server;

		serverListBox->Items->Append(item);
	}

	optionsPanel->Visibility = (servers->Size > 0 ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
}

/// <summary>
/// Invoked when the page unloads.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::SelectServer::pageRoot_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	UnregisterServerEvents();
}

/// <summary>
/// Invoked when the current item selection is changed on the server list box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::SelectServer::serverListBox_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		serverNameTextBox->Text			= server->Name;
		serverHostNameTextBox->Text		= server->Hostname;
		serverPortTextBox->Text			= server->Port;
		serverUsernameTextBox->Text		= server->Username;
		serverPasswordTextBox->Text		= server->Password;
		serverUserHostnameTextBox->Text = server->UserHostname;
		serverUserRealnameTextBox->Text = server->UserRealName;
		serverNameText->Text			= server->Name;

		optionsPanel->Visibility	   = Windows::UI::Xaml::Visibility::Visible;
		deleteServerButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
		
		RefreshServerInfo(false, false);
		
		RegisterServerEvents(server);
	}
	else
	{
		serverNameTextBox->Text			= "";
		serverHostNameTextBox->Text		= "";
		serverPortTextBox->Text			= "";
		serverUsernameTextBox->Text		= "";
		serverPasswordTextBox->Text		= "";
		serverUserHostnameTextBox->Text = "";
		serverUserRealnameTextBox->Text = "";
		serverNameText->Text			= "";
		
		optionsPanel->Visibility	   = Windows::UI::Xaml::Visibility::Collapsed;
		deleteServerButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

		UnregisterServerEvents();
	}
}

void IRCClient::SelectServer::RefreshServerInfo(bool error, bool byDisconnect)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
	
		if (error == true && byDisconnect == false)
		{
			connectToServerButton->Content = L"Failed To Connect";
		}
		else
		{
			if (server->Disconnecting == true)
			{
				connectToServerButton->Content = L"Disconnecting ...";
			}
			else if (server->Connected == false && server->Connecting == false)
			{
				connectToServerButton->Content = L"Connect To Server  \xE0AE";
			}
			else if (server->Connected == false)
			{
				connectToServerButton->Content = L"Connecting ...";
			}
			else
			{
				connectToServerButton->Content = L"Disconnect From Server \xE0D9";
			}
		}
	}
}

void IRCClient::SelectServer::OnServerConnectionFailure(LibIRC::IRCServer^ server)
{
	RefreshServerInfo(true, false);
}

void IRCClient::SelectServer::OnServerConnectionSuccess(LibIRC::IRCServer^ server)
{
	RefreshServerInfo(false, false);
}

void IRCClient::SelectServer::OnServerDisconnect(LibIRC::IRCServer^ server)
{
	RefreshServerInfo(true, true);
}

void IRCClient::SelectServer::serverNameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{	
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		server->Name = serverNameTextBox->Text;

		safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Content = server->Name;
		serverNameText->Text = server->Name;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::serverHostNameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
	
		server->Hostname = serverHostNameTextBox->Text;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::serverPortTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		server->Port = serverPortTextBox->Text;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::serverUsernameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		server->Username = serverUsernameTextBox->Text;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::serverPasswordTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		server->Password = serverPasswordTextBox->Text;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::serverUserHostnameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		server->UserHostname = serverUserHostnameTextBox->Text;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::serverUserRealnameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		server->UserRealName = serverUserHostnameTextBox->Text;

		App::Instance->SaveState();

		RefreshServerInfo(false, false);
	}
}

void IRCClient::SelectServer::addServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	LibIRC::IRCServer^ server = App::Instance->IRCController->CreateServer("irc.unknown.net", "6667");
	server->Name			  = "Unnamed Server";
	server->Username		  = "UnnamedUser";
	server->Password		  = "";
	server->UserHostname	  = "";
	server->UserRealName	  = "";

	ListBoxItem^ item = ref new ListBoxItem();
	item->Content = server->Name;
	item->IsSelected = true;
	item->Tag = server;

	serverListBox->Items->Append(item);
	serverListBox->SelectedItem = item;
	serverListBox->UpdateLayout();
	serverListBox->ScrollIntoView(item);

	App::Instance->SaveState();
}

void IRCClient::SelectServer::deleteServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		
		UnregisterServerEvents();

		unsigned int index;
		serverListBox->Items->IndexOf(serverListBox->SelectedItem, &index);
		serverListBox->Items->RemoveAt(index);

		if (serverListBox->Items->Size > 0)
		{
			if (index >= serverListBox->Items->Size)
			{
				serverListBox->SelectedItem = serverListBox->Items->GetAt(index - 1);
			}
			else
			{
				serverListBox->SelectedItem = serverListBox->Items->GetAt(index);
			}
		}

		App::Instance->IRCController->RemoveServer(server);
		App::Instance->SaveState();
	}
}

void IRCClient::SelectServer::serverNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverNameTextBox->Text == "")
	{
		serverNameTextBox->Text = "Unnamed Server";
	}
}

void IRCClient::SelectServer::serverHostNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverHostNameTextBox->Text == "")
	{
		serverHostNameTextBox->Text = "irc.unknown.net";
	}
}

void IRCClient::SelectServer::serverPortTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	String^ validNumber = "";
	for (unsigned int i = 0; i < serverPortTextBox->Text->Length(); i++)
	{
		wchar_t chr = serverPortTextBox->Text->Data()[i];
		if (chr >= '0' && chr <= '9')
		{
			validNumber += chr;
		}
	}

	if (validNumber == "")
	{
		validNumber = "6667";
	}

	serverPortTextBox->Text = validNumber;
}

void IRCClient::SelectServer::serverUsernameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverUsernameTextBox->Text == "")
	{
		serverUsernameTextBox->Text = "UnnamedUser";
	}
}

void IRCClient::SelectServer::connectToServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		
		if (server->Disconnecting == false)
		{
			if (server->Connected == false && server->Connecting == false)
			{
				connectToServerButton->Content = L"Connecting ...";
				server->AsyncConnect(false, 0);
			}
			else
			{
				connectToServerButton->Content = L"Disconnecting ...";
				server->AsyncDisconnect();
			}
		}
		else
		{
			server->ForceDisconnect();
		}
	}
}
