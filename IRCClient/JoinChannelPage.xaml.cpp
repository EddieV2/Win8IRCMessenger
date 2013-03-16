//
// JoinChannelPage.xaml.cpp
// Implementation of the JoinChannelPage class
//

#include "pch.h"
#include "JoinChannelPage.xaml.h"

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

JoinChannelPage::JoinChannelPage()
{
	InitializeComponent();

	JoinButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
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
void JoinChannelPage::LoadState(Object^ navigationParameter, IMap<String^, Object^>^ pageState)
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
void JoinChannelPage::SaveState(IMap<String^, Object^>^ pageState)
{
	(void) pageState;	// Unused parameter
}

/// <summary>
/// Invoked when the user clicks the join button.
/// </summary>
void IRCClient::JoinChannelPage::JoinButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	App::Instance->CurrentServer->AsyncJoinChannel(RoomNameTextBox->Text, PasswordTextBox->Text);
	this->Frame->GoBack();
}

/// <summary>
/// Invoked when the user changes the rooms name text.
/// </summary>
void IRCClient::JoinChannelPage::RoomNameTextBox_TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
	Platform::String^ str = "";
	bool ignored = false;

	// Check the room name is valid - remove any invalid characters.
	for (unsigned int i = 0; i < RoomNameTextBox->Text->Length(); i++)
	{
		wchar_t chr = RoomNameTextBox->Text->Data()[i];
		if ((chr >= 'A' && chr <= 'Z') ||
			(chr >= 'a' && chr <= 'z') ||
			(chr >= '0' && chr <= '9') ||
			(chr == '#' && i == 0))
		{
			str += chr;
		}
		else
		{
			ignored = true;
		}
	}

	// Make sure room starts with a hash makr.
	if (str->Data()[0] != '#')
	{
		str = "#" + str;
		ignored = true;
	}

	// Apply new formatted room name.
	RoomNameTextBox->Text = str;
	if (ignored == true)
	{
		RoomNameTextBox->SelectionStart = str->Length();
	}

	JoinButton->Visibility = (RoomNameTextBox->Text->Length() >= 2 ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed);
}
