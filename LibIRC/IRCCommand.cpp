// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "IRCCommand.h"

using namespace LibIRC;
using namespace Platform;
using namespace Platform::Collections;

IRCCommand::IRCCommand(String^ line)
{
	m_prefix			= "";
	m_command			= "";
	m_params			= ref new Vector<String^>();
	m_prefixNickname	= "";
	m_prefixUserName	= "";
	m_prefixHost		= "";

	ParseString(line);
}

IRCCommand::IRCCommand()
{
	m_prefix			= "";
	m_command			= "";
	m_params			= ref new Vector<String^>();
	m_prefixNickname	= "";
	m_prefixUserName	= "";
	m_prefixHost		= "";
}

IRCCommand::~IRCCommand()
{
	m_prefix			= nullptr;
	m_command			= nullptr;
	m_params			= nullptr;
	m_prefixNickname	= nullptr;
	m_prefixUserName	= nullptr;
	m_prefixHost		= nullptr;
}

/// <summary>
/// All of the following commands just create irc commands with varying numbers of parameters. They are simply time savers.
/// </summary>
IRCCommand^ IRCCommand::Create(Platform::String^ cmd)
{
	IRCCommand^ c	= ref new IRCCommand();
	c->m_command	= cmd;
	return c;
}
IRCCommand^ IRCCommand::Create(Platform::String^ cmd, Platform::String^ arg0)
{
	IRCCommand^ c	= ref new IRCCommand();
	c->m_command	= cmd;
	c->m_params->Append(arg0);
	return c;
}
IRCCommand^ IRCCommand::Create(Platform::String^ cmd, Platform::String^ arg0, Platform::String^ arg1)
{
	IRCCommand^ c	= ref new IRCCommand();
	c->m_command	= cmd;
	c->m_params->Append(arg0);
	c->m_params->Append(arg1);
	return c;
}
IRCCommand^ IRCCommand::Create(Platform::String^ cmd, Platform::String^ arg0, Platform::String^ arg1, Platform::String^ arg2)
{
	IRCCommand^ c	= ref new IRCCommand();
	c->m_command	= cmd;
	c->m_params->Append(arg0);
	c->m_params->Append(arg1);
	c->m_params->Append(arg2);
	return c;
}
IRCCommand^ IRCCommand::Create(Platform::String^ cmd, Platform::String^ arg0, Platform::String^ arg1, Platform::String^ arg2, Platform::String^ arg3)
{
	IRCCommand^ c	= ref new IRCCommand();
	c->m_command	= cmd;
	c->m_params->Append(arg0);
	c->m_params->Append(arg1);
	c->m_params->Append(arg2);
	c->m_params->Append(arg3);
	return c;
}

/// <summary>
/// Parses the given string representation of a command and stores it in this instance.
/// </summary>
void IRCCommand::ParseString(Platform::String^ line)
{
	unsigned int	offset = 0;
	const wchar_t*	data   = line->Data();

	m_prefix	= "";
	m_command	= "";
	m_params->Clear();

	// Read in the prefix.
	if (data[offset] == ':')
	{
		// Skip colon.
		offset++; 
		if (offset >= line->Length()) 
			return;

		// Keep reading till we get to a space.
		while (data[offset] != ' ') 
		{
			m_prefix += data[offset];
			offset++;

			if (offset >= line->Length()) 
				return;
		}

		// If we got a prefix, then split it into nick!user@host.	
		unsigned int	prefix_offset			= 0;
		const wchar_t*	prefix_data				= m_prefix->Data();
		int				prefix_parse_segment	= 0;

		while (prefix_offset < m_prefix->Length())
		{
			if (prefix_data[prefix_offset] == '!')
			{
				prefix_parse_segment = 1;
			}
			else if (prefix_data[prefix_offset] == '@')
			{				
				prefix_parse_segment = 2;
			}
			else
			{
				if (prefix_parse_segment == 0)
				{
					m_prefixNickname += prefix_data[prefix_offset];
				}
				else if (prefix_parse_segment == 1)
				{
					m_prefixUserName += prefix_data[prefix_offset];
				}
				else if (prefix_parse_segment == 2)
				{
					m_prefixHost += prefix_data[prefix_offset];
				}
			}

			prefix_offset++;
		}

	}

	// Read in the space.
	while (data[offset] == ' ') 
	{
		offset++;

		if (offset >= line->Length()) 
			return;
	}

	// Read in the command.
	wchar_t chr = data[offset];
	while ((data[offset] >= 'A' && data[offset] <= 'Z') ||
		   (data[offset] >= 'a' && data[offset] <= 'z') ||
		   (data[offset] >= '0' && data[offset] <= '9')) 
	{
		m_command += data[offset];
		offset++;

		if (offset >= line->Length()) 
			return;
	}

	// Read in the space.
	while (data[offset] == ' ') 
	{
		offset++;

		if (offset >= line->Length()) 
			return;
	}

	// Read in the parameters.
	bool readingTrailing = false;
	String^ partialParam = "";
	String^ trailing = "";

	while (offset < line->Length())
	{
		if (readingTrailing == true)
		{
			trailing += data[offset];
		}
		else
		{
			wchar_t chr = data[offset];
			if (chr == ':')
			{
				if (partialParam != "")
				{
					m_params->Append(partialParam);
					partialParam = "";
				}
				readingTrailing = true;
			}
			else if (chr == ' ')
			{
				if (partialParam != "")
				{
					m_params->Append(partialParam);
					partialParam = "";
				}
			}
			else
			{
				partialParam += chr;
			}
		}

		offset++;
	}

	if (partialParam != "")
	{
		m_params->Append(partialParam);
	}

	if (trailing != "")
	{
		m_params->Append(trailing);
	}
}

/// <summary>
/// Takes this commands information and formats it as a string.
/// </summary>
Platform::String^ IRCCommand::FormatAsString()
{
	String^ str = "";

	if (m_prefixNickname != "")
	{
		str += ":" + m_prefixNickname;
		if (m_prefixUserName != "")
		{
			str += "!" + m_prefixUserName;
		}
		if (m_prefixHost != "")
		{
			str += "@" + m_prefixHost;
		}
		str += " ";
	}

	str += m_command + " ";

	if (m_params->Size > 0)
	{

		// Append all the parameters.
		for (unsigned int i = 0; i < m_params->Size; i++)
		{
			if (i == m_params->Size - 1)
			{
				String^ param = m_params->GetAt(i);
				bool containsSpaces = false;

				for (unsigned int j = 0; j < param->Length(); j++)
				{
					if (param->Data()[j] == ' ')
					{
						containsSpaces = true;
						break;
					}
				}

				if (containsSpaces == true)
				{
					str += ":" + param;
				}
				else
				{
					str += param + " ";
				}
			}
			else
			{
				str += m_params->GetAt(i) + " ";
			}
		}
	}

	return str;
}
