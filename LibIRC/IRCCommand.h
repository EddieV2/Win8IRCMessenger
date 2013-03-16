// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include <pch.h>

namespace LibIRC
{
	// Forward references.
	ref class IRCServer;

	/// <summary>
	/// The IRC command class is used to encapsulate a command sent or recieved through
	/// and IRC connection.
	///
	///	Format of messages in BNF notation;
	///
	///	<message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
	///	<prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
	///	<command>  ::= <letter> { <letter> } | <number> <number> <number>
	///	<SPACE>    ::= ' ' { ' ' }
	///	<params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
	///
	///	<middle>   ::= <Any *non-empty* sequence of octets not including SPACE
	///				   or NUL or CR or LF, the first of which may not be ':'>
	///	<trailing> ::= <Any, possibly *empty*, sequence of octets not including
	///					 NUL or CR or LF>
	///
	///	<crlf>     ::= CR LF
	///
	/// <summary>
	public ref class IRCCommand sealed
	{
	private:
		Platform::String^									m_prefix;
		Platform::String^									m_prefixNickname;
		Platform::String^									m_prefixUserName;
		Platform::String^									m_prefixHost;
		Platform::String^									m_command;
		Platform::Collections::Vector<Platform::String^>^	m_params;

	public:
		
		/// <summary>
		/// Gets the prefixed server name (or other origin prefix).
		/// </summary>
		property Platform::String^ Prefix
		{
            Platform::String^ get()	{ return m_prefix; }
		}
		
		/// <summary>
		/// Gets the prefixed nick name.
		/// </summary>
		property Platform::String^ PrefixName
		{
            Platform::String^ get()	{ return m_prefixNickname; }
		}
		
		/// <summary>
		/// Gets the prefixed username.
		/// </summary>
		property Platform::String^ PrefixUserName
		{
            Platform::String^ get()	{ return m_prefixUserName; }
		}
		
		/// <summary>
		/// Gets the host prefix.
		/// </summary>
		property Platform::String^ PrefixHost
		{
            Platform::String^ get()	{ return m_prefixHost; }
		}
		
		/// <summary>
		/// Gets the actual command string, eg. NICK
		/// </summary>
		property Platform::String^ Command
		{
            Platform::String^ get()	{ return m_command; }
		}
		
		/// <summary>
		/// Gets an array of parameters for this command.
		/// </summary>
		property Windows::Foundation::Collections::IVector<Platform::String^>^ Parameters
		{
            Windows::Foundation::Collections::IVector<Platform::String^>^ get()	{ return m_params; }
		}
		
		// ------------------------------------------------------------------------------------
		// Public methods.
		// ------------------------------------------------------------------------------------
		IRCCommand(Platform::String^ line);
		IRCCommand();
		virtual ~IRCCommand();

		void				ParseString			(Platform::String^ line);
		Platform::String^	FormatAsString		();

		static IRCCommand^	Create				(Platform::String^ cmd);
		static IRCCommand^	Create				(Platform::String^ cmd, Platform::String^ arg0);
		static IRCCommand^	Create				(Platform::String^ cmd, Platform::String^ arg0, Platform::String^ arg1);
		static IRCCommand^	Create				(Platform::String^ cmd, Platform::String^ arg0, Platform::String^ arg1, Platform::String^ arg2);
		static IRCCommand^	Create				(Platform::String^ cmd, Platform::String^ arg0, Platform::String^ arg1, Platform::String^ arg2, Platform::String^ arg3);


	};

}