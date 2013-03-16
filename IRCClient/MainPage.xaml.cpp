// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------

#include "pch.h"
#include "MainPage.xaml.h"
#include "SelectServer.xaml.h"
#include "JoinChannelPage.xaml.h"
#include "HelpContents.xaml.h"

using namespace IRCClient;

using namespace Platform;
using namespace Concurrency;
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
using namespace Windows::System::Threading;
using namespace Windows::UI::ApplicationSettings;
using namespace Windows::UI::Popups;

MainPage::MainPage()
{
	InitializeComponent();

	m_eventHookedIRCServers				= ref new Platform::Collections::Vector<EventHookedIRCServer^>();
	m_eventHookedIRCChannels			= ref new Platform::Collections::Vector<EventHookedIRCChannel^>();

	m_refreshWindowCounter				= 0;
}

/// <summary>
/// Invoked when the page loads.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::Page_Loaded_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	HookServerEvents();
	RefreshServerWindow(true);
}

/// <summary>
/// Invoked when the page unloads.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::Page_Unloaded_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	UnhookServerEvents();
}
/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

/// <summary>
/// Invoked when the user clicks the select a server button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::SelectAServerButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	UnhookServerEvents();
    this->Frame->Navigate(TypeName(SelectServer::typeid));
}

/*
/// <summary>
/// Invoked when the user clicks the "close server" button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::OnServerCloseButtonTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	EventHookedIRCServer^ server = safe_cast<EventHookedIRCServer^>(safe_cast<Button^>(sender)->Tag);
	if (server->Server->Disconnecting == true)
	{
		server->Server->ForceDisconnect();
	}
	else
	{
		server->Server->AsyncDisconnect();
	}
	RefreshServerWindow();
}
*/

/// <summary>
/// Invoked when the user clicks the server button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::OnServerTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	EventHookedIRCServer^ server = safe_cast<EventHookedIRCServer^>(safe_cast<Grid^>(sender)->Tag);
	App::Instance->CurrentServer = server->Server;
	RefreshServerWindow(true);
}

/*
/// <summary>
/// Invoked when the user clicks the "close channel" button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::OnChannelCloseButtonTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	EventHookedIRCChannel^ channel = safe_cast<EventHookedIRCChannel^>(safe_cast<Button^>(sender)->Tag);
	// TODO: Leave channel.
	RefreshServerWindow(true);
}
*/

/// <summary>
/// Invoked when the user clicks the channel button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::OnChannelTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	EventHookedIRCChannel^ channel = safe_cast<EventHookedIRCChannel^>(safe_cast<Grid^>(sender)->Tag);
	App::Instance->CurrentChannel = channel->Channel;
	RefreshServerWindow(true);
}

/// <summary>
/// Refreshes the server window.
/// </summary>
void IRCClient::MainPage::RefreshServerWindow(bool refreshUserList)
{	
	// ---------------------------------------------------------------------
	// Server is null? Find one to select.
	// ---------------------------------------------------------------------

	// Look for current server to make sure we can use it.
	bool			   requireNewServer			= true;
	LibIRC::IRCServer^ availableToSelectServer	= nullptr;

	for (unsigned int i = 0; i < m_eventHookedIRCServers->Size; i++)
	{
		EventHookedIRCServer^ server = m_eventHookedIRCServers->GetAt(i);
		if (server->Server->Active == false)
		{
			continue;
		}

		if (availableToSelectServer == nullptr)
		{
			availableToSelectServer = server->Server;
		}

		if (App::Instance->CurrentServer == server->Server)
		{
			requireNewServer = false;
			break;
		}
	}

	if (requireNewServer == true)
	{
		App::Instance->CurrentServer = availableToSelectServer;
	}
	
	// ---------------------------------------------------------------------
	// Room is null? Find one to select.
	// ---------------------------------------------------------------------
	
	if (App::Instance->CurrentServer != nullptr)
	{
		// Look for current channel/room to make sure we can use it.
		bool			    requireNewChannel			= true;
		LibIRC::IRCChannel^ availableToSelectChannel	= nullptr;

		for (unsigned int i = 0; i < App::Instance->CurrentServer->Channels->Size; i++)
		{
			LibIRC::IRCChannel^ channel = App::Instance->CurrentServer->Channels->GetAt(i);
			
			if (availableToSelectChannel == nullptr)
			{
				availableToSelectChannel = channel;
			}
			
			if (App::Instance->CurrentChannel == channel)
			{
				requireNewChannel = false;
				break;
			}
		}

		if (requireNewChannel == true)
		{
			App::Instance->CurrentChannel = availableToSelectChannel;
		}
	}
	
	// ---------------------------------------------------------------------
	// Work out which kind of view to show.
	// ---------------------------------------------------------------------
	if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
	{
		if (App::Instance->CurrentChannel->Type != LibIRC::IRCChannelType::IRC_CHANNEL_TYPE_NORMAL)
		{
			SubMode = "DummyWindow";
		}
		else
		{
			SubMode = "NormalWindow";
		}
	}
	else
	{		
		SubMode = "";
	}
	
	// ---------------------------------------------------------------------
	// Update the visibility of icons on the app bar.
	// ---------------------------------------------------------------------
	bool isSnapped = (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped);

	JoinButton->Visibility				= (App::Instance->CurrentServer != nullptr ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
	LeaveButton->Visibility				= (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr && App::Instance->CurrentChannel->Name != "Server" ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
	DisconnectButton->Visibility		= (isSnapped == false && App::Instance->CurrentServer != nullptr ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
	HelpButton->Visibility				= (isSnapped == false ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
	
	// ---------------------------------------------------------------------
	// Refresh the server and channel lists.
	// ---------------------------------------------------------------------

	// This tiny bit of code prevents us from reloading the window multiple
	// times when multiple events are fired in succession.
	m_refreshWindowCounter++;
	unsigned int counter = m_refreshWindowCounter;

	RunUIAfterDelay(20, ref new Windows::UI::Core::DispatchedHandler([this, refreshUserList, counter] ()
	{

		// If another request for a refresh is already in the works - ignore this one.
		if (refreshUserList == true && counter < m_refreshWindowCounter)
		{
			return;
		}

		// Refresh the window. 
		// We use a delay because the width/height properties will return old values until
		// they have been rendered.

		RefreshServerList();
		RefreshRoomList();
		RefreshMessageList(refreshUserList);

	}, Platform::CallbackContext::Any));
}

/// <summary>
/// Refreshes the message list.
/// </summary>
void IRCClient::MainPage::RefreshMessageList(bool refreshUserList)
{	
	bool isSnapped = (Windows::UI::ViewManagement::ApplicationView::Value == Windows::UI::ViewManagement::ApplicationViewState::Snapped);

	// Grab some things we will need to use.
	Windows::UI::Xaml::Media::FontFamily^ monospaceFont = ref new Windows::UI::Xaml::Media::FontFamily("Courier New");

	// Is this the first time we are setting up the mesage stack?
	bool firstRun = (messageStackPanel->Children->Size == 0);

	// Delete old messages.
	while (messageStackPanel->Children->Size > 0)
	{
		Windows::UI::Xaml::UIElement^ element = messageStackPanel->Children->GetAt(0);
		messageStackPanel->Children->RemoveAt(0);
		delete element;
	}

	// Delete old users.
	if (refreshUserList == true)
	{
		while (userScrollViewer->Items->Size > 0)
		{
			Platform::Object^ element = userScrollViewer->Items->GetAt(0);
			userScrollViewer->Items->RemoveAt(0);
			delete element;
		}
	}
	
	// Create new messages.
	if (App::Instance->CurrentChannel != nullptr)
	{
		double fullHeight = 0;

		messageStackPanel->Margin = Windows::UI::Xaml::Thickness(0, 0, 0, 0);
		messageStackPanel->Width = max(10, messageScrollViewer->ActualWidth - 10);

		for (unsigned int i = max(0, ((int)App::Instance->CurrentChannel->Messages->Size) - MAX_MESSAGE_TO_SHOW_AT_ONCE); i < App::Instance->CurrentChannel->Messages->Size; i++)
		{
			LibIRC::IRCMessage^ message = App::Instance->CurrentChannel->Messages->GetAt(i);

			// Add some padding to the top so that the first message always appears at the bottom of the screen.
			double topPadding = 0;
			if (i == 0)
			{
				topPadding = messageScrollViewer->ActualHeight - 25;
			}

			// Work out the username / message we will be showing.
			Platform::String^ usernameText = "";
			Platform::String^ messageText  = "";
			Windows::UI::Xaml::Media::SolidColorBrush^ backgroundColor = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::MidnightBlue);
			Platform::String^ friendlyUsername = message->User != nullptr ? message->User->GetFriendlyTextForChannel(App::Instance->CurrentChannel) : "";

			// Work out what we need to display, and in what style.
			switch (message->Type)
			{
				case LibIRC::IRCMessageType::IRC_MESSAGE_TYPE_ACTION:
					usernameText = "";
					messageText = friendlyUsername + " " + message->Friendly;
					backgroundColor = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 156, 0, 156));
					break;
				
				case LibIRC::IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE:
					usernameText = "";
					messageText = message->Friendly;
					backgroundColor = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 147, 0));
					break;
				
				case LibIRC::IRCMessageType::IRC_MESSAGE_TYPE_NORMAL:
					usernameText = message->User == nullptr ? "" : "<" + friendlyUsername + ">";
					messageText = message->Friendly;
					backgroundColor = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 65, 65, 65));
					break;
				
				case LibIRC::IRCMessageType::IRC_MESSAGE_TYPE_NOTICE:
					usernameText = message->User == nullptr ? "" : "<" + friendlyUsername + ">";
					messageText = message->Friendly;
					backgroundColor = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 127, 0, 0));
					break;

				case LibIRC::IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE:
					usernameText = "";
					messageText = message->Friendly;
					backgroundColor = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::ColorHelper::FromArgb(255, 0, 0, 127));
					break;
			}

			// This is a bit of a hack, we need to measure the text using an autosize text label so we can create the correct
			// sized message boxes.
			textMeasurementBox->Margin				= Windows::UI::Xaml::Thickness(0, 0, 0, 0);
			textMeasurementBox->TextWrapping		= Windows::UI::Xaml::TextWrapping::Wrap;
			textMeasurementBox->MaxWidth			= 100000;
			textMeasurementBox->MaxHeight			= 9999999;
			textMeasurementBox->Text				= usernameText;
			textMeasurementBox->FontSize			= 16;
			textMeasurementBox->FontWeight			= Windows::UI::Text::FontWeights::Bold;
			textMeasurementBox->FontFamily			= monospaceFont;
			textMeasurementBox->TextAlignment		= Windows::UI::Xaml::TextAlignment::Left;
			textMeasurementBox->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
			textMeasurementBox->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
			textMeasurementBox->UpdateLayout();
			double usernameWidth = textMeasurementBox->ActualWidth;
			double usernameHeight = textMeasurementBox->ActualHeight;

			if (usernameText == "")
			{
				usernameHeight = 0;
			}
			
			textMeasurementBox->Margin				= Windows::UI::Xaml::Thickness(0, 0, 0, 0);
			textMeasurementBox->TextWrapping		= Windows::UI::Xaml::TextWrapping::Wrap;
			if (isSnapped == true)
			{
				textMeasurementBox->MaxWidth			= max(10, messageStackPanel->Width - 10);
			}
			else
			{
				textMeasurementBox->MaxWidth			= max(10, messageStackPanel->Width - usernameWidth - 30);
			}
			textMeasurementBox->MaxHeight			= 9999999;
			textMeasurementBox->Text				= messageText;
			textMeasurementBox->FontSize			= 16;
			textMeasurementBox->FontFamily			= monospaceFont;
			textMeasurementBox->FontWeight			= Windows::UI::Text::FontWeights::Normal;
			textMeasurementBox->TextAlignment		= Windows::UI::Xaml::TextAlignment::Left;
			textMeasurementBox->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
			textMeasurementBox->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
			textMeasurementBox->UpdateLayout();
			double messageWidth = textMeasurementBox->ActualWidth;
			double messageHeight = textMeasurementBox->ActualHeight;

			// Remove text so it dosen't show up on screen :3.
			textMeasurementBox->Text		= "";

			// Create the main grid our text area will be inserted into.
			Grid^ grid					= ref new Grid();
			grid->Margin				= Windows::UI::Xaml::Thickness(0, topPadding, 0, 2);
			grid->Width					= messageStackPanel->Width;
			if (isSnapped == true)
			{
				grid->Height				= usernameHeight + messageHeight + 10;
			}
			else
			{
				grid->Height				= messageHeight + 10;
			}
			grid->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
			grid->VerticalAlignment		= Windows::UI::Xaml::VerticalAlignment::Top;
			grid->Background			= backgroundColor;
			messageStackPanel->Children->Append(grid);

			// Make the username label.
			TextBlock^ textBlock			= ref new TextBlock();
			//textBlock->Margin				= Windows::UI::Xaml::Thickness(10, 10, 5, 5);
			textBlock->Margin				= Windows::UI::Xaml::Thickness(5, 5, 5, 5);
			textBlock->Width				= usernameWidth;
			textBlock->Height				= 20;
			textBlock->FontSize				= 16;
			textBlock->FontWeight			= Windows::UI::Text::FontWeights::Bold;
			textBlock->Text					= usernameText;
			textBlock->FontFamily			= monospaceFont;
			textBlock->TextAlignment		= Windows::UI::Xaml::TextAlignment::Left;
			textBlock->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
			textBlock->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
			textBlock->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightPink);

			grid->Children->Append(textBlock);
			
			// Make the message label.
			textBlock						= ref new TextBlock();
			//textBlock->Margin				= Windows::UI::Xaml::Thickness(10 + usernameWidth + 10, 10, 0, 0);
			if (isSnapped == true)
			{
				textBlock->Margin				= Windows::UI::Xaml::Thickness(5, 5 + usernameHeight, 0, 0);
			}
			else
			{
				textBlock->Margin				= Windows::UI::Xaml::Thickness(5 + usernameWidth + 5, 5, 0, 0);
			}
			textBlock->TextWrapping			= Windows::UI::Xaml::TextWrapping::Wrap;
			textBlock->FontWeight			= Windows::UI::Text::FontWeights::Normal;
			textBlock->FontSize				= 16;
			textBlock->Width				= messageWidth + 1; // Round up to make sure we don't wrap.
			textBlock->Height				= messageHeight;
			textBlock->Text					= messageText;
			textBlock->FontFamily			= monospaceFont;
			textBlock->TextAlignment		= Windows::UI::Xaml::TextAlignment::Left;
			textBlock->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
			textBlock->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
			textBlock->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
			grid->Children->Append(textBlock);

			fullHeight += grid->Height + grid->Margin.Bottom + grid->Margin.Top;
		}
		
		messageStackPanel->Height = fullHeight;
		messageScrollViewer->UpdateLayout();

		// Update message stack panel and scroll to bottom if we are already scrolled to near the end.
		//float internalHeight = messageScrollViewer->ActualHeight - messageStackPanel->ActualHeight;
		//float scrollOffset = messageScrollViewer->VerticalOffset;
		//float fromEnd = messageStackPanel->Height - messageScrollViewer->VerticalOffset;
		//if (fromEnd < 100 || firstRun == true) 
		//{
		messageScrollViewer->ScrollToVerticalOffset(messageStackPanel->ActualHeight);
		//}
	}
	
	if (refreshUserList == true)
	{
		// Create new users.
		if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr && App::Instance->CurrentChannel->Type == LibIRC::IRCChannelType::IRC_CHANNEL_TYPE_NORMAL)
		{
			for (unsigned int i = 0; i < App::Instance->CurrentServer->Users->Size; i++)
			{
				LibIRC::IRCUser^ user = App::Instance->CurrentServer->Users->GetAt(i);

				// Ignore users that are not inside this channel.
				if (!user->IsActiveInChannel(App::Instance->CurrentChannel))
				{
					continue;
				}

				// Work out the username / message we will be showing.
				Platform::String^ usernameText = user->GetFriendlyTextForChannel(App::Instance->CurrentChannel);

				// Make the username label.
				ListViewItem^ item			= ref new ListViewItem();

				item->Margin				= Windows::UI::Xaml::Thickness(0, 0, 0, 0);
				item->FontSize				= 18;
				item->Height				= 30;
				item->FontWeight			= Windows::UI::Text::FontWeights::Bold;
				item->Content				= usernameText;
				item->FontFamily			= monospaceFont;
				item->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
				item->VerticalAlignment		= Windows::UI::Xaml::VerticalAlignment::Top;
				item->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
				item->RightTapped			+= ref new RightTappedEventHandler(this, &IRCClient::MainPage::userScrollViewer_ItemClick);
				item->Tapped				+= ref new TappedEventHandler(this, &IRCClient::MainPage::userScrollViewer_ItemClickTapped);
				userScrollViewer->Items->Append(item);

			}

		}
	}
}

/// <summary>
/// Refreshes the server list in the top app bar.
/// </summary>
void IRCClient::MainPage::RefreshServerList()
{
	// Delete old gadgets.
	for (unsigned int i = 0; i < m_eventHookedIRCServers->Size; i++)
	{
		EventHookedIRCServer^ server = m_eventHookedIRCServers->GetAt(i);
		unsigned int index = 0;

		if (server->NameTextBlockControl != nullptr)
		{			
			server->GridControl->Children->IndexOf(server->NameTextBlockControl, &index);
			server->GridControl->Children->RemoveAt(index);

			delete server->NameTextBlockControl;
			server->NameTextBlockControl = nullptr;
		}

		if (server->StatusTextBlockControl != nullptr)
		{			
			server->GridControl->Children->IndexOf(server->StatusTextBlockControl, &index);
			server->GridControl->Children->RemoveAt(index);

			delete server->StatusTextBlockControl;
			server->StatusTextBlockControl = nullptr;
		}

		if (server->CloseButtonControl != nullptr)
		{		
			server->GridControl->Children->IndexOf(server->CloseButtonControl, &index);
			server->GridControl->Children->RemoveAt(index);

			delete server->CloseButtonControl;
			server->CloseButtonControl = nullptr;
		}

		if (server->GridControl != nullptr)
		{			
			ServersGridView->Children->IndexOf(server->GridControl, &index);
			ServersGridView->Children->RemoveAt(index);

			delete server->GridControl;
			server->GridControl = nullptr;
		}
	}

	// Create new gadgets.
	unsigned int leftOffset  = 0;
	unsigned int borderWidth = 10;
	for (unsigned int i = 0; i < m_eventHookedIRCServers->Size; i++)
	{
		EventHookedIRCServer^ server = m_eventHookedIRCServers->GetAt(i);
		if (server->Server->Active == false)
		{
			continue;
		}
		
		// Create the main grid our server box will show on.
		Grid^ grid					= ref new Grid();
		server->GridControl			= grid;
		grid->Margin				= Windows::UI::Xaml::Thickness(leftOffset, 6, 0, 0);
		grid->Width					= 150;
		grid->Height				= 49;
		grid->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		grid->VerticalAlignment		= Windows::UI::Xaml::VerticalAlignment::Top;
		grid->Tag					= server;
		grid->Tapped				+= ref new Windows::UI::Xaml::Input::TappedEventHandler(this, &IRCClient::MainPage::OnServerTapped);
		ServersGridView->Children->Append(grid);

		// Change background color depending on if its selected or not.
		if (App::Instance->CurrentServer == server->Server)
		{
			grid->Background			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightBlue);
		}
		else
		{
			grid->Background			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightGray);
		}

		// Main name label.
		TextBlock^ nameLabel			= ref new TextBlock();
		server->NameTextBlockControl	= nameLabel;
		nameLabel->FontSize				= 14;
		nameLabel->FontWeight			= Windows::UI::Text::FontWeights::Bold;
		nameLabel->Margin				= Windows::UI::Xaml::Thickness(6, 7, 0, 0);
		nameLabel->Width				= 111;
		nameLabel->Height				= 16;
		nameLabel->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		nameLabel->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
		nameLabel->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
		nameLabel->Text					= server->Server->Name;
		grid->Children->Append(nameLabel);

		// Main status label.
		nameLabel						= ref new TextBlock();
		server->StatusTextBlockControl	= nameLabel;
		nameLabel->Margin				= Windows::UI::Xaml::Thickness(11, 8 + 20, 0, 0);
		nameLabel->Width				= 111;
		nameLabel->Height				= 16;
		nameLabel->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		nameLabel->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
		nameLabel->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
		nameLabel->Text					= "" + server->Server->Channels->Size + " rooms";
		grid->Children->Append(nameLabel);
		
		// Close button.
		/*
		Button^ button				= ref new Button();
		server->CloseButtonControl	= button;
		button->Margin				= Windows::UI::Xaml::Thickness(130 - 20, -2, 0, 0);
		button->Width				= 45;
		button->Height				= 49 + 4;
		button->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		button->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
		button->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
		button->BorderThickness		= 0;
		button->FontFamily			= ref new Windows::UI::Xaml::Media::FontFamily("Segoe UI Symbol");
		button->Content				= L"\xE10A";
		button->Tag					= server;
		button->Tapped				+= ref new Windows::UI::Xaml::Input::TappedEventHandler(this, &IRCClient::MainPage::OnServerCloseButtonTapped);
		grid->Children->Append(button);
		*/

		leftOffset += 150 + borderWidth;
	}
	ServersGridView->Width = max(1, leftOffset - borderWidth);
}

/// <summary>
/// Refrehes the channel/room list in the top app bar.
/// </summary>
void IRCClient::MainPage::RefreshRoomList()
{
	// Delete old gadgets.
	for (unsigned int i = 0; i < m_eventHookedIRCChannels->Size; i++)
	{
		EventHookedIRCChannel^ server = m_eventHookedIRCChannels->GetAt(i);
		unsigned int index = 0;

		if (server->NameTextBlockControl != nullptr)
		{			
			server->GridControl->Children->IndexOf(server->NameTextBlockControl, &index);
			server->GridControl->Children->RemoveAt(index);

			delete server->NameTextBlockControl;
			server->NameTextBlockControl = nullptr;
		}

		if (server->StatusTextBlockControl != nullptr)
		{			
			server->GridControl->Children->IndexOf(server->StatusTextBlockControl, &index);
			server->GridControl->Children->RemoveAt(index);

			delete server->StatusTextBlockControl;
			server->StatusTextBlockControl = nullptr;
		}

		if (server->CloseButtonControl != nullptr)
		{		
			server->GridControl->Children->IndexOf(server->CloseButtonControl, &index);
			server->GridControl->Children->RemoveAt(index);

			delete server->CloseButtonControl;
			server->CloseButtonControl = nullptr;
		}

		if (server->GridControl != nullptr)
		{			
			RoomsGridView->Children->IndexOf(server->GridControl, &index);
			RoomsGridView->Children->RemoveAt(index);

			delete server->GridControl;
			server->GridControl = nullptr;
		}
	}

	// Remove hooked channels that no longer exist.
	for (unsigned int i = 0; i < m_eventHookedIRCChannels->Size; i++)
	{
		EventHookedIRCChannel^ channel = m_eventHookedIRCChannels->GetAt(i);
		bool exists = false;

		// Check if it exists!
		for (unsigned int j = 0; j < m_eventHookedIRCServers->Size; j++)
		{
			EventHookedIRCServer^ server = m_eventHookedIRCServers->GetAt(j);
			if (server->Server->Active == false)
			{
				continue;
			}

			for (unsigned int k = 0; k < server->Server->Channels->Size; k++)
			{
				LibIRC::IRCChannel^ checkChannel = server->Server->Channels->GetAt(k);
				if (channel->Channel == checkChannel)
				{
					exists = true;
					break;
				}
			}
		}

		// Remove channel.
		if (exists == false)
		{
			m_eventHookedIRCChannels->RemoveAt(i);
			i--; 
			continue;
		}
	}

	// Add hooked channels that we've just joined.
	for (unsigned int j = 0; j < m_eventHookedIRCServers->Size; j++)
	{
		EventHookedIRCServer^ server = m_eventHookedIRCServers->GetAt(j);
		if (server->Server->Active == false)
		{
			continue;
		}

		for (unsigned int k = 0; k < server->Server->Channels->Size; k++)
		{
			LibIRC::IRCChannel^ channel = server->Server->Channels->GetAt(k);
			bool exists = false;

			for (unsigned int i = 0; i < m_eventHookedIRCChannels->Size; i++)
			{
				EventHookedIRCChannel^ checkChannel = m_eventHookedIRCChannels->GetAt(i);
				if (checkChannel->Channel == channel)
				{
					exists = true;
					break;
				}
			}

			// Add the new hooked channel.
			if (exists == false)
			{
				EventHookedIRCChannel^ newChannel = ref new EventHookedIRCChannel();
				newChannel->Channel = channel;
				newChannel->Server  = server->Server;

				m_eventHookedIRCChannels->Append(newChannel);
			}
		}
	}

	// Create the new gadgets.
	unsigned int leftOffset  = 0;
	unsigned int borderWidth = 10;
	for (unsigned int i = 0; i < m_eventHookedIRCChannels->Size; i++)
	{
		EventHookedIRCChannel^ channel = m_eventHookedIRCChannels->GetAt(i);
		if (channel->Server != App::Instance->CurrentServer)
		{
			continue;
		}
		if (channel->Channel == App::Instance->CurrentChannel)
		{
			channel->Channel->ResetNewMessageCount();
		}
		
		// Create the main grid our server box will show on.
		Grid^ grid					= ref new Grid();
		channel->GridControl		= grid;
		grid->Margin				= Windows::UI::Xaml::Thickness(leftOffset, 6, 0, 0);
		grid->Width					= 150;
		grid->Height				= 49;
		grid->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		grid->VerticalAlignment		= Windows::UI::Xaml::VerticalAlignment::Top;
		grid->Tag					= channel;
		grid->Tapped				+= ref new Windows::UI::Xaml::Input::TappedEventHandler(this, &IRCClient::MainPage::OnChannelTapped);
		RoomsGridView->Children->Append(grid);

		// Change background color depending on if its selected or not.
		if (App::Instance->CurrentChannel == channel->Channel)
		{
			grid->Background			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightBlue);
		}
		else
		{
			grid->Background			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightGray);
		}

		// Main name label.
		TextBlock^ nameLabel			= ref new TextBlock();
		channel->NameTextBlockControl	= nameLabel;
		nameLabel->FontSize				= 14;
		nameLabel->FontWeight			= Windows::UI::Text::FontWeights::Bold;
		nameLabel->Margin				= Windows::UI::Xaml::Thickness(6, 7, 0, 0);
		nameLabel->Width				= 140;
		nameLabel->Height				= 16;
		nameLabel->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		nameLabel->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
		nameLabel->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
		nameLabel->Text					= channel->Channel->Name;
		grid->Children->Append(nameLabel);

		// Main status label.
		nameLabel						= ref new TextBlock();
		channel->StatusTextBlockControl	= nameLabel;
		nameLabel->Margin				= Windows::UI::Xaml::Thickness(11, 8 + 20, 0, 0);
		nameLabel->Width				= 111;
		nameLabel->Height				= 16;
		nameLabel->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
		nameLabel->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;

		if (channel->Channel->NewMessageCount > 0)
		{
			nameLabel->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Red);
			nameLabel->FontWeight			= Windows::UI::Text::FontWeights::Bold;
		}
		else
		{
			nameLabel->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
		}

		nameLabel->Text					= "" + channel->Channel->NewMessageCount + " new messages";
		grid->Children->Append(nameLabel);
		
		// Close button. (Don't allow closing of the internal "server" channel).
		/*
		if (channel->Channel->Name != "Server")
		{
			Button^ button				= ref new Button();
			channel->CloseButtonControl	= button;
			button->Margin				= Windows::UI::Xaml::Thickness(130 - 20, -2, 0, 0);
			button->Width				= 45;
			button->Height				= 49 + 4;
			button->HorizontalAlignment	= Windows::UI::Xaml::HorizontalAlignment::Left;
			button->VerticalAlignment	= Windows::UI::Xaml::VerticalAlignment::Top;
			button->Foreground			= ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
			button->BorderThickness		= 0;
			button->FontFamily			= ref new Windows::UI::Xaml::Media::FontFamily("Segoe UI Symbol");
			button->Content				= L"\xE10A";
			button->Tag					= channel;
			button->Tapped				+= ref new Windows::UI::Xaml::Input::TappedEventHandler(this, &IRCClient::MainPage::OnChannelCloseButtonTapped);
			grid->Children->Append(button);
		}
		*/

		leftOffset += 150 + borderWidth;
	}
	RoomsGridView->Width = max(100, leftOffset - borderWidth);
}

/// <summary>
/// Invoked when the connection state of any server changes.
/// </summary>
void IRCClient::MainPage::ServerConnectionEvent(LibIRC::IRCServer^ server)
{
	RefreshServerWindow(true);
}

/// <summary>
/// Invoked when a user joins a channel.
/// </summary>
void IRCClient::MainPage::ServerChannelJoinEvent(LibIRC::IRCServer^ server, LibIRC::IRCChannel^ channel, LibIRC::IRCUser^ user)
{
	if (user->NickName == server->CurrentNickName)
	{
		App::Instance->CurrentServer = server;
		App::Instance->CurrentChannel = channel;
	}

	if (App::Instance->CurrentChannel == channel)
	{
		RefreshServerWindow(true);
	}
}

/// <summary>
/// Invoked when the connection state of any server changes.
/// </summary>
void IRCClient::MainPage::ServerChannelEvent(LibIRC::IRCServer^ server, LibIRC::IRCChannel^ channel, LibIRC::IRCUser^ user)
{
	if (App::Instance->CurrentChannel == channel)
	{
		RefreshServerWindow(true);
	}
}

/// <summary>
/// Invoked when a server recieves a message.
/// </summary>
void IRCClient::MainPage::ServerMessageRecievedEvent(LibIRC::IRCServer^ server, LibIRC::IRCMessage^ message)
{
	if (App::Instance->CurrentChannel == message->Origin || message->Origin == nullptr)
	{
		RefreshServerWindow(false);
	}
	else
	{
		RefreshServerList();
		RefreshRoomList();
	}
}

/// <summary>
/// Invoked when a user changes its nickname.
/// </summary>
void IRCClient::MainPage::ServerUserNicknameChangedEvent(LibIRC::IRCServer^ server, LibIRC::IRCUser^ user)
{	
	RefreshServerWindow(true);
}

/// <summary>
/// Invoked when a user has its channel mode changed..
/// </summary>
void IRCClient::MainPage::ServerUserChannelModeChangedEvent(LibIRC::IRCServer^ server, LibIRC::IRCChannel^ channel, LibIRC::IRCUser^ user)
{
	RefreshServerWindow(true);
}


/// <summary>
/// Hooks all events we are listening for on servers.
/// </summary>
void IRCClient::MainPage::HookServerEvents()
{
	LibIRC::IRCController^ controller = App::Instance->IRCController;
	for (unsigned int i = 0; i < controller->GetServerList()->Size; i++)
	{
		LibIRC::IRCServer^ server = controller->GetServerList()->GetAt(i);
		if (server != nullptr)
		{
			EventHookedIRCServer^ hook = ref new EventHookedIRCServer();
			hook->Server = server;
			hook->ServerConnectFailureEventCookie			= server->OnConnectFailure				+= ref new LibIRC::IRCConnectFailureDelegate(this, &IRCClient::MainPage::ServerConnectionEvent);
			hook->ServerConnectSuccessEventCookie			= server->OnConnectSuccess				+= ref new LibIRC::IRCConnectSuccessDelegate(this, &IRCClient::MainPage::ServerConnectionEvent);
			hook->ServerConnectDisconnectEventCookie		= server->OnDisconnect					+= ref new LibIRC::IRCDisconnectDelegate(this, &IRCClient::MainPage::ServerConnectionEvent);
			hook->ServerJoinChannelEventCookie				= server->OnJoinChannel					+= ref new LibIRC::IRCJoinChannelDelegate(this, &IRCClient::MainPage::ServerChannelJoinEvent);
			hook->ServerLeaveChannelEventCookie				= server->OnLeaveChannel				+= ref new LibIRC::IRCLeaveChannelDelegate(this, &IRCClient::MainPage::ServerChannelEvent);
			hook->ServerMessageRecievedEventCookie			= server->OnMessageRecieved				+= ref new LibIRC::IRCMessageRecievedDelegate(this, &IRCClient::MainPage::ServerMessageRecievedEvent);
			hook->ServerUserChannelModeChangedEventCookie	= server->OnUserChannelModeChanged		+= ref new LibIRC::IRCUserChannelModeChangedDelegate(this, &IRCClient::MainPage::ServerUserChannelModeChangedEvent);
			hook->ServerUserNicknameChangedEventCookie		= server->OnUserNicknameChanged			+= ref new LibIRC::IRCUserNicknameChangedDelegate(this, &IRCClient::MainPage::ServerUserNicknameChangedEvent);
			m_eventHookedIRCServers->Append(hook);
		}
	}

}

/// <summary>
/// Unhooks all events we are listening for on servers.
/// </summary>
void IRCClient::MainPage::UnhookServerEvents()
{
	// Clear old hooked events.
	for (unsigned int i = 0; i < m_eventHookedIRCServers->Size; i++)
	{
		EventHookedIRCServer^ server = m_eventHookedIRCServers->GetAt(i);
		server->Server->OnConnectFailure			-= server->ServerConnectFailureEventCookie;
		server->Server->OnConnectSuccess			-= server->ServerConnectSuccessEventCookie;
		server->Server->OnDisconnect				-= server->ServerConnectDisconnectEventCookie;
		server->Server->OnJoinChannel				-= server->ServerJoinChannelEventCookie;
		server->Server->OnLeaveChannel				-= server->ServerLeaveChannelEventCookie;
		server->Server->OnMessageRecieved			-= server->ServerMessageRecievedEventCookie;
		server->Server->OnUserChannelModeChanged	-= server->ServerUserChannelModeChangedEventCookie;
		server->Server->OnUserNicknameChanged		-= server->ServerUserNicknameChangedEventCookie;
		server->Server = nullptr;
	}
	m_eventHookedIRCServers->Clear();
	m_eventHookedIRCChannels->Clear();
}

/// <summary>
/// Invoked when the user clicks the join channel button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::JoinButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	UnhookServerEvents();
    this->Frame->Navigate(TypeName(JoinChannelPage::typeid));
}

/// <summary>
/// Invoked when the user clicks the leave channel button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::LeaveButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	LibIRC::IRCChannel^ channel = App::Instance->CurrentChannel;
	channel->AsyncLeave();
	RefreshServerWindow(true);
}

/// <summary>
/// Invoked when the user clicks the disconnect button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::DisconnectButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	LibIRC::IRCServer^ server = App::Instance->CurrentServer;
	if (server->Disconnecting == true)
	{
		server->ForceDisconnect();
	}
	else
	{
		server->AsyncDisconnect();
	}
	RefreshServerWindow(true);
}

/// <summary>
/// Invoked when the user clicks the settings button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::SettingsButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{	
	UnhookServerEvents();
    this->Frame->Navigate(TypeName(SelectServer::typeid));
}

/// <summary>
/// Invoked when the user clicks the help button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::HelpButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	UnhookServerEvents();
    this->Frame->Navigate(TypeName(HelpContents::typeid));
}

/// <summary>
/// Invoked when the user presses a button in the say text box.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::sayTextBox_KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
	if (e->Key == Windows::System::VirtualKey::Enter)
	{
		SendMessage();
	}
}

/// <summary>
/// Invoked when the user presses the send button.
/// </summary>
/// <param name="e">The control that invoked this event.</param>
/// <param name="e">Event data that describes how this event was reached.</param>
void IRCClient::MainPage::sendButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	SendMessage();
}

/// <summary>
/// Sends the message the user has input into the text box.
/// </summary>
void IRCClient::MainPage::SendMessage()
{
	Platform::String^ msg = sayTextBox->Text;
	if (msg != "" && App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
	{
		App::Instance->CurrentServer->ParseAndSendMessage(App::Instance->CurrentChannel, msg);
	}
	sayTextBox->Text = "";
}

/// <summary>
/// Runs UI code after a given delay. This is primarily used as a work around for when an event is called (eg. size changed) which
///	causes the screen state to change next render. This allows us to run the code after the next render so size (width/height) properties
///	return the correct values.
/// </summary>
void IRCClient::MainPage::RunUIAfterDelay(unsigned int delay, Windows::UI::Core::DispatchedHandler^ handler)
{
	// Event to be throw when timer fires.
	task_completion_event<void> complete_event;

	// Create one-shot timer.
	Concurrency::timer<int>* fire_once = new timer<int>(delay, 0, nullptr, false);

	// Create a callback.
	auto callback = new call<int>([complete_event](int)
	{
		complete_event.set();						
	});

	// Link everything up.
	fire_once->link_target(callback);
	fire_once->start();

	// Wait until we are done!
	task<void>(complete_event).then([this, callback, fire_once, delay, handler]()
	{
		delete callback;
		delete fire_once;

		// Make sure we execute UI stuff on the UI thread, or things will complain.
		Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, handler);
	});
}

/// <summary>
/// Invoked when the size of the page changes (resolution change, visual state change, etc)
/// </summary>
void IRCClient::MainPage::Page_SizeChanged_1(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{	
	RefreshServerWindow(true);
}

/// <summary>
/// Invoked when the user clicks an item in the scroll viewer.
/// </summary>
void IRCClient::MainPage::userScrollViewer_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Input::RightTappedRoutedEventArgs^ e)
{
	if (App::Instance->CurrentServer == nullptr ||
		App::Instance->CurrentChannel == nullptr)
	{
		return;
	}

	auto menu = ref new Windows::UI::Popups::PopupMenu();
	auto item = safe_cast<ListViewItem^>(sender);
	auto name = safe_cast<Platform::String^>(item->Content);

	LibIRC::IRCUser^ user = App::Instance->CurrentServer->GetUserByName(name);
	if (user == nullptr)
	{
		return;
	}

	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("Query", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{		
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{				
				// Query user and change to new channel for user.
				App::Instance->CurrentChannel = App::Instance->CurrentServer->QueryUser(user);

				// Make sure we run the window update on the UI thread.
				RunUIAfterDelay(20, ref new Windows::UI::Core::DispatchedHandler([this] ()
				{
					RefreshServerWindow(true);
				}, Platform::CallbackContext::Any));
			}
		})));

	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("+ Op", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{	
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncOpUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("+ Voice", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{	
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncVoiceUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("- All", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{		
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{			
				App::Instance->CurrentChannel->AsyncDePrivilageUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("Kick", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{			
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncKickUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("Ban", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{	
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncBanUser(user);
			}
		})));

	menu->ShowAsync(e->GetPosition(this));
}

/// <summary>
/// Invoked when the user clicks an item in the scroll viewer.
/// </summary>
void IRCClient::MainPage::userScrollViewer_ItemClickTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	if (App::Instance->CurrentServer == nullptr ||
		App::Instance->CurrentChannel == nullptr)
	{
		return;
	}

	auto menu = ref new Windows::UI::Popups::PopupMenu();
	auto item = safe_cast<ListViewItem^>(sender);
	auto name = safe_cast<Platform::String^>(item->Content);

	LibIRC::IRCUser^ user = App::Instance->CurrentServer->GetUserByName(name);
	if (user == nullptr)
	{
		return;
	}

	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("Query", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{		
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{				
				// Query user and change to new channel for user.
				App::Instance->CurrentChannel = App::Instance->CurrentServer->QueryUser(user);

				// Make sure we run the window update on the UI thread.
				RunUIAfterDelay(20, ref new Windows::UI::Core::DispatchedHandler([this] ()
				{
					RefreshServerWindow(true);
				}, Platform::CallbackContext::Any));
			}
		})));

	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("+ Op", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{	
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncOpUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("+ Voice", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{	
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncVoiceUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("- All", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{		
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{			
				App::Instance->CurrentChannel->AsyncDePrivilageUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("Kick", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{			
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncKickUser(user);
			}
		})));
	
	menu->Commands->Append(ref new Windows::UI::Popups::UICommand("Ban", ref new Windows::UI::Popups::UICommandInvokedHandler([this, user](Windows::UI::Popups::IUICommand^ command)
		{	
			if (App::Instance->CurrentServer != nullptr && App::Instance->CurrentChannel != nullptr)
			{
				App::Instance->CurrentChannel->AsyncBanUser(user);
			}
		})));

	menu->ShowAsync(e->GetPosition(this));
}
