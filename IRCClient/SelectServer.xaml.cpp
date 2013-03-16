//
// SelectServer.xaml.cpp
// Implementation of the SelectServer class
//

#include "pch.h"
#include "SelectServer.xaml.h"
#include "EditServer.xaml.h"

using namespace IRCClient;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

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
/// Refreshes the server information.
/// </summary>
void IRCClient::SelectServer::RefreshServerInfo(bool error, bool byDisconnect)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
	
		if (error == true && byDisconnect == false)
		{
			connectServerButton->Content = L"Failed To Connect";
			connectServerButton->IsEnabled = true;
		}
		else
		{
			if (server->Disconnecting == true)
			{
				connectServerButton->Content = L"Disconnecting ...";
				connectServerButton->IsEnabled = false;
			}
			else if (server->Connected == false && server->Connecting == false)
			{
				connectServerButton->Content = L"\xE0AE  Connect";
				connectServerButton->IsEnabled = true;
			}
			else if (server->Connected == false)
			{
				connectServerButton->Content = L"Connecting ...";
				connectServerButton->IsEnabled = false;
			}
			else
			{
				connectServerButton->Content = L"\xE0D9  Disconnect";				
				connectServerButton->IsEnabled = true;
			}
		}
	}
}

/// <summary>
/// Invoked when a server connection fails.
/// </summary>
void IRCClient::SelectServer::OnServerConnectionFailure(LibIRC::IRCServer^ server)
{
	RefreshServerInfo(true, false);
}

/// <summary>
/// Invoked when a server connection passes.
/// </summary>
void IRCClient::SelectServer::OnServerConnectionSuccess(LibIRC::IRCServer^ server)
{
	RefreshServerInfo(false, false);
}

/// <summary>
/// Invoked when a server connection disconnects.
/// </summary>
void IRCClient::SelectServer::OnServerDisconnect(LibIRC::IRCServer^ server)
{
	RefreshServerInfo(true, true);
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

		// Delete button for servers is not visible in snapper view.
		if (Windows::UI::ViewManagement::ApplicationView::Value != Windows::UI::ViewManagement::ApplicationViewState::Snapped)
		{
			deleteServerButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
		}
		editServerButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
		
		RefreshServerInfo(false, false);		
		RegisterServerEvents(server);
	}
	else
	{
		// Delete button is not visible in snapped view.
		if (Windows::UI::ViewManagement::ApplicationView::Value != Windows::UI::ViewManagement::ApplicationViewState::Snapped)
		{
			deleteServerButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		}
		editServerButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

		UnregisterServerEvents();
	}
}

/// <summary>
/// Invoked when the user clicks the add server button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
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

	App::Instance->CurrentEditServer = server;
    this->Frame->Navigate(TypeName(EditServer::typeid));
}

/// <summary>
/// Invoked when the user clicks the delete server button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::SelectServer::deleteServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		
		UnregisterServerEvents();

		// Remove server from list box.
		unsigned int index;
		serverListBox->Items->IndexOf(serverListBox->SelectedItem, &index);
		serverListBox->Items->RemoveAt(index);

		// Select the next server in the list.
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

		// Remove from irc controller and save our current state.
		App::Instance->IRCController->RemoveServer(server);
		App::Instance->SaveState();
	}
}

/// <summary>
/// Invoked when the user clicks the edit server button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::SelectServer::editServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{		
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		
		App::Instance->CurrentEditServer = server;

		this->Frame->Navigate(TypeName(EditServer::typeid));
	}
}

/// <summary>
/// Invoked when the user clicks the connect to server button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::SelectServer::connectServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (serverListBox->SelectedItem != nullptr)
	{
		LibIRC::IRCServer^ server = safe_cast<LibIRC::IRCServer^>(safe_cast<ListBoxItem^>(serverListBox->SelectedItem)->Tag);
		
		if (App::Instance->CheckForMeteredNetwork() == false)
		{
			if (server->Disconnecting == false)
			{
				if (server->Connected == false && server->Connecting == false)
				{
					connectServerButton->Content = L"Connecting ...";
					connectServerButton->IsEnabled = false;
					server->AsyncConnect(false, 0);
				}
				else
				{
					connectServerButton->Content = L"Disconnecting ...";
					connectServerButton->IsEnabled = false;
					server->AsyncDisconnect();
				}
			}
			else
			{
				server->ForceDisconnect();
			}
		}
	}
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

	// Delete button is not visible in snapped state.
	if (Windows::UI::ViewManagement::ApplicationView::Value != Windows::UI::ViewManagement::ApplicationViewState::Snapped)
	{
		deleteServerButton->Visibility = (servers->Size > 0 ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
	}
	editServerButton->Visibility   = (servers->Size > 0 ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
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
