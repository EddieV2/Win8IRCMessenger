// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "IRCMessage.h"
#include "IRCUser.h"

using namespace LibIRC;
using namespace Platform;
using namespace Platform::Collections;

IRCMessage::IRCMessage(IRCChannel^ origin, IRCCommand^ command, IRCMessageType type, Platform::String^ friendly)
{
	m_origin		= origin;
	m_command		= command;
	m_friendly		= friendly;
	m_messageType	= type;
	m_user			= nullptr;

	// Is this an "action" message?
	if (m_friendly->Length() > 7 && 
		m_friendly->Data()[0] == 1 &&
		m_friendly->Data()[1] == 'A' &&
		m_friendly->Data()[2] == 'C' &&
		m_friendly->Data()[3] == 'T' &&
		m_friendly->Data()[4] == 'I' &&
		m_friendly->Data()[5] == 'O' &&
		m_friendly->Data()[6] == 'N')
	{
		Platform::String^ finalData = "";
		for (unsigned int i = 8; i < m_friendly->Length() - 1; i++)
		{
			finalData += m_friendly->Data()[i];
		}

		m_friendly = finalData;
		m_messageType = IRCMessageType::IRC_MESSAGE_TYPE_ACTION;
	}
}

IRCMessage::IRCMessage(IRCChannel^ origin, IRCCommand^ command, IRCMessageType type, Platform::String^ friendly, IRCUser^ user)
{
	m_origin		= origin;
	m_command		= command;
	m_friendly		= friendly;
	m_messageType	= type;
	m_user			= user;

	// Is this an "action" message?
	if (m_friendly->Length() > 7 && 
		m_friendly->Data()[0] == 1 &&
		m_friendly->Data()[1] == 'A' &&
		m_friendly->Data()[2] == 'C' &&
		m_friendly->Data()[3] == 'T' &&
		m_friendly->Data()[4] == 'I' &&
		m_friendly->Data()[5] == 'O' &&
		m_friendly->Data()[6] == 'N')
	{
		Platform::String^ finalData = "";
		for (unsigned int i = 8; i < m_friendly->Length() - 1; i++)
		{
			finalData += m_friendly->Data()[i];
		}

		m_friendly = finalData;
		m_messageType = IRCMessageType::IRC_MESSAGE_TYPE_ACTION;
	}
}

IRCMessage::~IRCMessage()
{
	m_origin	= nullptr;
	m_command	= nullptr;
	m_friendly	= nullptr;
	m_user		= nullptr;
}
