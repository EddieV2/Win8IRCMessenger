// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "MainPage.xaml.h"

using namespace IRCClient;

using namespace Platform;
using namespace Windows::Storage;
using namespace Platform::Collections;
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
using namespace Windows::UI::ApplicationSettings;
using namespace Windows::UI::Popups;
using namespace Windows::Networking::Connectivity;

// Static variables!
App^ App::m_instance = nullptr;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);

	// Store App class so other users can access our irc related bits and pieces.
	m_instance = this;

	// Create IRC Controller.
	m_controller = ref new LibIRC::IRCController(); 
	m_currentServer = nullptr;
	m_currentChannel = nullptr;
	m_recievedNetworkOptIn = false;

	// Load current state of app.
	LoadState();
}

/// <summary>
/// Invoked when the window is created.
/// </summary>
void App::OnWindowCreated(WindowCreatedEventArgs^ args)
{
	// Settings pane.
	m_commandsRequestedToken = SettingsPane::GetForCurrentView()->CommandsRequested += ref new TypedEventHandler<SettingsPane^, SettingsPaneCommandsRequestedEventArgs^>(this, &IRCClient::App::OnSettingsPaneCommandRequested); 
	m_settingsEventRegistered = true;

	// Register for network status changes.
	NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler(this, &IRCClient::App::OnNetworkStatusChanged);
}

/// <summary>
/// Invoked when then network status changes.
/// </summary>
void App::OnNetworkStatusChanged(Platform::Object^ sender)
{
	CheckForMeteredNetwork();
}

/// <summary>
/// Checks for a metered connections - if we have one it recieves opt-in consent from the user.
/// </summary>
/// <returns>True if opt-in is required, else False</returns>
bool App::CheckForMeteredNetwork()
{
	if (m_recievedNetworkOptIn == true)
	{
		return false;
	}

	NetworkCostType costType = NetworkInformation::GetInternetConnectionProfile()->GetConnectionCost()->NetworkCostType;
	bool roaming			 = NetworkInformation::GetInternetConnectionProfile()->GetConnectionCost()->Roaming;

	if (costType == NetworkCostType::Fixed || costType == NetworkCostType::Variable || roaming == true)
	{
		// Make sure we execute UI stuff on the UI thread, or things will complain.
		Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this] ()
		{
			
			MessageDialog^ dialog = ref new MessageDialog("This application requires a constant internet connection.\n\nA constant connection may cause data usage to go over your cap on this metered network.\n\nBefore you will be able to connect to any IRC servers you must allow this application constant access to the internet on this connection.\n\nDo you want to allow access?", "");

			// Add dialog commands. Lambda functions ftw!
			dialog->Commands->Append(ref new UICommand("Allow Access", ref new UICommandInvokedHandler([this](IUICommand^ command) 
			{ 
				m_recievedNetworkOptIn = true;
			}))); 

			// Awwwwh, no access allowed :(
			dialog->Commands->Append(ref new UICommand("Decline Access", ref new UICommandInvokedHandler([this](IUICommand^ command) 
			{ 
				m_recievedNetworkOptIn = false;

				auto servers = m_controller->GetServerList();
				for (unsigned int i = 0; i < servers->Size; i++)
				{
					auto server = servers->GetAt(i);
					if (server->Connected == true || server->Connecting == true)
					{
						server->ForceDisconnect();
					}
				}
			}))); 

			dialog->DefaultCommandIndex = 0;
			dialog->CancelCommandIndex = 1;

			dialog->ShowAsync();

		}, Platform::CallbackContext::Any));

		return true;
	}

	return false;
}

/// <summary>
/// Invoked when a settings pane commands are requested.
/// </summary>
void App::OnSettingsPaneCommandRequested(SettingsPane^ sender, SettingsPaneCommandsRequestedEventArgs^ args)
{ 
	UICommandInvokedHandler^ handler = ref new UICommandInvokedHandler(this, &App::DoSettingsPaneOperation); 
 
    SettingsCommand^ generalCommand = ref new SettingsCommand("privacyButton", "Privacy Statement", handler); 
    args->Request->ApplicationCommands->Append(generalCommand); 
}

/// <summary>
/// Invoked when the user selects a settings pane operation.
/// </summary>
void App::DoSettingsPaneOperation(Windows::UI::Popups::IUICommand^ command)
{
	Uri^ uri = ref new Uri("http://twindrills.com/irc-messenger/privacy-policy.html");
	Windows::System::Launcher::LaunchUriAsync(uri);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="args">Details about the launch request and process.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args)
{
	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// Do not repeat app initialization when the Window already has content,
	// just ensure that the window is active
	if (rootFrame == nullptr)
	{
		// Create a Frame to act as the navigation context and associate it with
		// a SuspensionManager key
		rootFrame = ref new Frame();

		if (args->PreviousExecutionState == ApplicationExecutionState::Terminated)
		{
			// TODO: Restore the saved session state only when appropriate, scheduling the
			// final launch steps after the restore is complete

		}

		if (rootFrame->Content == nullptr)
		{
			// When the navigation stack isn't restored navigate to the first page,
			// configuring the new page by passing required information as a navigation
			// parameter
			if (!rootFrame->Navigate(TypeName(MainPage::typeid), args->Arguments))
			{
				throw ref new FailureException("Failed to create initial page");
			}

			CheckForMeteredNetwork();
		}
		// Place the frame in the current Window
		Window::Current->Content = rootFrame;
		// Ensure the current window is active
		Window::Current->Activate();
	}
	else
	{
		if (rootFrame->Content == nullptr)
		{
			// When the navigation stack isn't restored navigate to the first page,
			// configuring the new page by passing required information as a navigation
			// parameter
			if (!rootFrame->Navigate(TypeName(MainPage::typeid), args->Arguments))
			{
				throw ref new FailureException("Failed to create initial page");
			}
		}
		// Ensure the current window is active
		Window::Current->Activate();
	}
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter

	SaveState();
}

/// <summary>
/// Saves the state of the application.
/// </summary>
void App::SaveState()
{
	// Get container to store settings in.
	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;

	// Get list of servers.
	IVector<LibIRC::IRCServer^>^ servers =  m_controller->GetServerList();

	localSettings->Values->Insert("/servers/count", PropertyValue::CreateUInt32(servers->Size));
	for (unsigned int i = 0; i < servers->Size; i++)
	{
		LibIRC::IRCServer^ server = servers->GetAt(i);

		localSettings->Values->Insert("/servers/" + i + "/hostname",	PropertyValue::CreateString(server->Hostname));
		localSettings->Values->Insert("/servers/" + i + "/port",		PropertyValue::CreateString(server->Port));
		localSettings->Values->Insert("/servers/" + i + "/name",		PropertyValue::CreateString(server->Name));
		localSettings->Values->Insert("/servers/" + i + "/username",	PropertyValue::CreateString(server->Username));
		localSettings->Values->Insert("/servers/" + i + "/password",	PropertyValue::CreateString(server->Password));
		localSettings->Values->Insert("/servers/" + i + "/userhostname",PropertyValue::CreateString(server->UserHostname));
		localSettings->Values->Insert("/servers/" + i + "/userrealname",PropertyValue::CreateString(server->UserRealName));
	}
}

/// <summary>
/// Loads the state of the application.
/// </summary>
void App::LoadState()
{
	// Get container to store settings in.
	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
	
	// Start creating servers.
	unsigned int server_count = 0;
	if (localSettings->Values->HasKey("/servers/count"))
	{
		server_count = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/count"))->GetUInt32();
	}

	// If we have no servers, load the default ones.
	if (server_count == 0)
	{
		LoadDefaultServers();
	}

	// Else load them.
	else
	{
		for (unsigned int i = 0; i < server_count; i++)
		{
			String^ hostname	 = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/hostname"))->GetString();
			String^ port		 = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/port"))->GetString();
			String^ name		 = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/name"))->GetString();
			String^ username	 = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/username"))->GetString();
			String^ password	 = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/password"))->GetString();
			String^ userhostname = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/userhostname"))->GetString();
			String^ userrealname = safe_cast<IPropertyValue^>(localSettings->Values->Lookup("/servers/" + i + "/userrealname"))->GetString();
			
			LibIRC::IRCServer^ server = m_controller->CreateServer(hostname, port);
			server->Name			= name;
			server->Username		= username;
			server->Password		= password;
			server->UserHostname	= userhostname;
			server->UserRealName	= userrealname;
		}
	}
}

/// <summary>
/// Fills up the server list with a few default servers to make things look a little less empty :P.
/// </summary>
void App::LoadDefaultServers()
{
	LibIRC::IRCServer^ server;

	// Harvested these servers from the top-10 at http://irc.netsplit.de/networks/IRCnet/
	
	server				= m_controller->CreateServer("irc.kbfail.net", "6667");
	server->Name		= "KeyboardFail";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.quakenet.org", "6667");
	server->Name		= "QuakeNet";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.undernet.org", "6667");
	server->Name		= "Undernet";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.servercentral.net", "6667");
	server->Name		= "EFNet";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.rizon.net", "6667");
	server->Name		= "Rizon";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("c.ustream.tv", "6667");
	server->Name		= "Ustream";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.irc-hispano.org", "6667");
	server->Name		= "IRC-Hispano";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.dal.net", "6667");
	server->Name		= "DALnet";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.chatzona.org", "6667");
	server->Name		= "ChatZona";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.oftc.net", "6667");
	server->Name		= "OFTC";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.gamesurge.net", "6667");
	server->Name		= "GameSurge";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.irchighway.net", "6667");
	server->Name		= "IRCHighway";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.swiftirc.net", "6667");
	server->Name		= "SwiftIRC";
	server->Username	= "UnnamedUser";

	server				= m_controller->CreateServer("irc.freenode.net", "6667");
	server->Name		= "Freenode";
	server->Username	= "UnnamedUser";
}