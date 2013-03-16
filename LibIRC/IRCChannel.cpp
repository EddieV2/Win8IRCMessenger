// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "IRCChannel.h"
#include "IRCMessage.h"
#include "IRCServer.h"
#include "IRCCommand.h"

using namespace LibIRC;
using namespace Platform;

IRCChannel::IRCChannel(IRCServer^ server, String^ name, IRCChannelType type)
	: m_server(server),
	  m_name(name),
	  m_channelType(type)
{	
	m_messages = ref new Platform::Collections::Vector<IRCMessage^>();
}

IRCChannel::~IRCChannel()
{
	m_server	= nullptr;
	m_name		= nullptr;
	m_messages	= nullptr;
	m_lastSeenMessageCount = 0;
}

/// <summary>
/// Adds the given message to this channel.
/// </summary>
void IRCChannel::AddMessage(IRCMessage^ message)
{
	// Add new messages.
	m_messages->Append(message);

	// Remove any old messages.
	while (m_messages->Size > IRC_CHANNEL_MAX_MESSAGES_TO_STORE)
	{
		m_messages->RemoveAt(0);
	}

	m_messagesRecieved += 1;
}

/// <summary>
/// Asyncronously leaves this channel.
/// </summary>
void IRCChannel::AsyncLeave()
{	
	if (m_channelType == IRCChannelType::IRC_CHANNEL_TYPE_USER)
	{
		m_server->DisposeChannel(this, true);
	}
	else
	{
		m_server->SendCommand(IRCCommand::Create("PART", m_name));
	}
}

/// <summary>
/// Resets the number of messages that have not been read on this channel.
/// </summary>
void IRCChannel::ResetNewMessageCount()
{
	m_lastSeenMessageCount = m_messagesRecieved;
}

/// <summary>
/// Asyncronously sets a user as a operator.
/// </summary>
void IRCChannel::AsyncOpUser(IRCUser^ user)
{
	m_server->SendCommand(IRCCommand::Create("MODE", m_name, "+o", user->NickName));
}

/// <summary>
/// Asyncronously sets a user as a voice.
/// </summary>
void IRCChannel::AsyncVoiceUser(IRCUser^ user)
{
	m_server->SendCommand(IRCCommand::Create("MODE", m_name, "+v", user->NickName));
}

/// <summary>
/// Asyncronously removes all privilages from a user.
/// </summary>
void IRCChannel::AsyncDePrivilageUser(IRCUser^ user)
{
	if (user->IsChannelModeSet(this, 'o'))
	{
		m_server->SendCommand(IRCCommand::Create("MODE", m_name, "-o", user->NickName));
	}
	if (user->IsChannelModeSet(this, 'v'))
	{
		m_server->SendCommand(IRCCommand::Create("MODE", m_name, "-v", user->NickName));
	}
}

/// <summary>
/// Asyncronously kicks a user from ths channel.
/// </summary>
void IRCChannel::AsyncKickUser(IRCUser^ user)
{
	m_server->SendCommand(IRCCommand::Create("KICK", m_name, user->NickName));
}

/// <summary>
/// Asyncronously bans a user from ths channel.
/// </summary>
void IRCChannel::AsyncBanUser(IRCUser^ user)
{
	m_server->SendCommand(IRCCommand::Create("MODE", m_name, "+b", user->NickName));
}