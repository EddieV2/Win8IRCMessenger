// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "IRCUser.h"

using namespace LibIRC;
using namespace Platform;

IRCUserChannelMode::IRCUserChannelMode(IRCChannel^ channel, Platform::String^ mode)
{
	m_channel = channel;
	m_mode = mode;
}

IRCUser::IRCUser(IRCServer^ server, String^ name, Platform::String^ host, Platform::String^ nickname)
	: m_server(server),
	  m_name(name),
	  m_host(host),
	  m_nickName(nickname)
{
	m_activeChannels   = ref new Platform::Collections::Vector<IRCChannel^>();
	m_userChannelModes = ref new Platform::Collections::Vector<IRCUserChannelMode^>();
}

IRCUser::~IRCUser()
{
	m_server	= nullptr;
	m_name		= nullptr;
	m_host		= nullptr;
	m_nickName	= nullptr;

	m_activeChannels->Clear();
	m_activeChannels = nullptr;

	m_userChannelModes->Clear();
	m_userChannelModes = nullptr;
}

/// <summary>
/// Flags the user as active in the given channel.
/// </summary>
void IRCUser::AddActiveChannel(IRCChannel^ channel)
{
	unsigned int index = 0;

	if (m_activeChannels->IndexOf(channel, &index) != true)
	{
		m_activeChannels->Append(channel);
		m_userChannelModes->Append(ref new IRCUserChannelMode(channel, ""));
	}
}

/// <summary>
/// Flags the user as no longer active in the given channel.
/// </summary>
void IRCUser::RemoveActiveChannel(IRCChannel^ channel)
{
	unsigned int index = 0;

	if (m_activeChannels->IndexOf(channel, &index) == true)
	{
		m_activeChannels->RemoveAt(index);
	}

	for (unsigned int i = 0; i < m_userChannelModes->Size; i++)
	{
		IRCUserChannelMode^ channelMode = m_userChannelModes->GetAt(i);
		if (channelMode->Channel == channel)
		{
			m_userChannelModes->RemoveAt(i);
			i--;
			continue;
		}
	}
}

/// <summary>
/// Sets the users per-channel mode for the given channel.
/// </summary>
void IRCUser::SetChannelMode(IRCChannel^ channel, Platform::String^ mode)
{
	for (unsigned int i = 0; i < m_userChannelModes->Size; i++)
	{
		IRCUserChannelMode^ channelmode = m_userChannelModes->GetAt(i);
		if (channelmode->Channel == channel)
		{

			wchar_t op_mode  = mode->Data()[0];
			wchar_t mode_chr = mode->Data()[1];

			if (op_mode == '-')
			{
				if (!IsChannelModeSet(channel, mode_chr))
				{
					return;
				}

				Platform::String^ new_mode = "";
				for (unsigned int i = 0; i < channelmode->Mode->Length(); i++)
				{
					wchar_t chr = channelmode->Mode->Data()[i];
					if (chr != mode_chr)
					{
						new_mode += chr; 
					}
				}

				channelmode->Mode = new_mode;
			}
			else if (op_mode == '+')
			{
				if (IsChannelModeSet(channel, mode_chr))
				{
					return;
				}

				channelmode->Mode += mode_chr;
			}
			else
			{
				channelmode->Mode = mode;
			}

		}
	}
}

/// <summary>
/// Gets the users per-channel mode for the given channel.
/// </summary>
Platform::String^ IRCUser::GetChannelMode(IRCChannel^ channel)
{
	for (unsigned int i = 0; i < m_userChannelModes->Size; i++)
	{
		IRCUserChannelMode^ mode =	m_userChannelModes->GetAt(i);
		if (mode->Channel == channel)
		{
			return mode->Mode;
		}
	}
	return "";
}

/// <summary>
/// Gets if the user has a given mode flag applied to them for the given channel.
/// </summary>
bool IRCUser::IsChannelModeSet(IRCChannel^ channel, wchar_t mode_chr)
{
	for (unsigned int i = 0; i < m_userChannelModes->Size; i++)
	{
		IRCUserChannelMode^ mode =	m_userChannelModes->GetAt(i);
		if (mode->Channel == channel)
		{
			for (unsigned int i = 0; i < mode->Mode->Length(); i++)
			{
				if (mode->Mode->Data()[i] == mode_chr)
				{
					return true;
				}
			}
		}
	}
	return false;
}

/// <summary>
/// Sets the users global mode.
/// </summary>
void IRCUser::SetMode(Platform::String^ mode)
{
	wchar_t op_mode  = mode->Data()[0];
	wchar_t mode_chr = mode->Data()[1];

	if (op_mode == '-')
	{
		if (!IsModeSet(mode_chr))
		{
			return;
		}

		Platform::String^ new_mode = "";
		for (unsigned int i = 0; i < m_mode->Length(); i++)
		{
			wchar_t chr = m_mode->Data()[i];
			if (chr != mode_chr)
			{
				new_mode += chr; 
			}
		}

		m_mode = new_mode;
	}
	else if (op_mode == '+')
	{
		if (IsModeSet(mode_chr))
		{
			return;
		}

		m_mode += mode_chr;
	}
	else
	{
		m_mode = mode;
	}
}

/// <summary>
/// Gets the users global mode.
/// </summary>
Platform::String^ IRCUser::GetMode()
{
	return m_mode;
}

/// <summary>
/// Gets if the user as a mode flag set globally.
/// </summary>
bool IRCUser::IsModeSet(wchar_t mode)
{
	for (unsigned int i = 0; i < m_mode->Length(); i++)
	{
		if (m_mode->Data()[i] == mode)
		{
			return true;
		}
	}

	return false;
}

/// <summary>
/// Gets a friendly text version of this users username for the given channel. This mainly just adds the ~&@%+ symbols onto the front of their name.
/// </summary>
Platform::String^ IRCUser::GetFriendlyTextForChannel(IRCChannel^ channel)
{
	Platform::String^ friendly = m_nickName;

	// Privilage level prefixes.
	// As the user can have several of these, but only one is ever shown
	// we only use the highest privilage prefix.
	if (IsChannelModeSet(channel, 'q')) // Owner
	{
		friendly = "~" + friendly;
	}
	else if (IsChannelModeSet(channel, 'a')) // Admin
	{
		friendly = "&" + friendly;
	}
	else if (IsChannelModeSet(channel, 'o')) // Operator
	{
		friendly = "@" + friendly;
	}
	else if (IsChannelModeSet(channel, 'h')) // Half-Op
	{
		friendly = "%" + friendly;
	}
	else if (IsChannelModeSet(channel, 'v')) // Voice
	{
		friendly = "+" + friendly;
	}

	return friendly;
}

/// <summary>
/// Returns true if the user is active in the given channel.
/// </summary>
bool IRCUser::IsActiveInChannel(IRCChannel^ channel)
{
	unsigned int index = 0;
	return (m_activeChannels->IndexOf(channel, &index) == true);
}
