//
// EditServer.xaml.cpp
// Implementation of the EditServer class
//

#include "pch.h"
#include "EditServer.xaml.h"

using namespace IRCClient;

using namespace Platform;
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

EditServer::EditServer()
{
	InitializeComponent();
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
void EditServer::LoadState(Object^ navigationParameter, IMap<String^, Object^>^ pageState)
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
void EditServer::SaveState(IMap<String^, Object^>^ pageState)
{
	(void) pageState;	// Unused parameter
}

/// <summary>
/// Invoked when the user changes control from the name text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverNameTextBox->Text == "")
	{
		serverNameTextBox->Text = "Unnamed Server";
	}

	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->Name = serverNameTextBox->Text;
	
	pageTitle->Text	= "Edit " + server->Name;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the user changes control from the host text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverHostNameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverHostNameTextBox->Text == "")
	{
		serverHostNameTextBox->Text = "irc.unknown.net";
	}

	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->Hostname = serverHostNameTextBox->Text;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the user changes control from the port text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverPortTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
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

	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->Port = serverPortTextBox->Text;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the user changes control from the username text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverUsernameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	if (serverUsernameTextBox->Text == "")
	{
		serverUsernameTextBox->Text = "UnnamedUser";
	}

	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->Username = serverUsernameTextBox->Text;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the user changes control from the password text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverPasswordTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->Password = serverPasswordTextBox->Text;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the user changes control from the user hostname text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverUserHostnameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->UserHostname = serverUserHostnameTextBox->Text;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the user changes control from the user realname text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::serverUserRealnameTextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;
	server->UserRealName = serverUserHostnameTextBox->Text;

	App::Instance->SaveState();
}

/// <summary>
/// Invoked when the page is first loaded.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::EditServer::pageRoot_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{	
	LibIRC::IRCServer^ server = App::Instance->CurrentEditServer;

	serverNameTextBox->Text				= server->Name;
	pageTitle->Text						= "Edit " + server->Name;
	serverHostNameTextBox->Text			= server->Hostname;
	serverPortTextBox->Text				= server->Port;
	serverUsernameTextBox->Text			= server->Username;
	serverPasswordTextBox->Text			= server->Password;
	serverUserHostnameTextBox->Text		= server->UserHostname;
	serverUserRealnameTextBox->Text		= server->UserRealName;
}
