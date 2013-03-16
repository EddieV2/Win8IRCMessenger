// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include <pch.h>
#include "IRCMessage.h"

namespace LibIRC
{
	// Forward references.
	ref class IRCServer;
	ref class IRCMessage;
	
	/// <summary>
	/// Dictates what kind of mode a channel is (user / room).
	/// <summary>
	public enum class IRCChannelType : int
	{
		IRC_CHANNEL_TYPE_NORMAL,
		IRC_CHANNEL_TYPE_USER,
		IRC_CHANNEL_TYPE_DUMMY,
	};

	// The maximum number of messages to store.
	#define IRC_CHANNEL_MAX_MESSAGES_TO_STORE 1000

	/// <summary>
	/// The IRC channel class manages code access to a specific channel on a given IRCServer.
	///	Contains all the code allowing the user to do any channel specific operations.
	/// <summary>
	public ref class IRCChannel sealed
	{
	private:
		IRCServer^									m_server;
		Platform::String^							m_name;
		IRCChannelType								m_channelType;

		Platform::Collections::Vector<IRCMessage^>^ m_messages;
		unsigned int								m_messagesRecieved;

		bool										m_parsingUserList;
		
		unsigned int								m_lastSeenMessageCount;

	public:
		
		// ------------------------------------------------------------------------------------
		// Public properties.
		// ------------------------------------------------------------------------------------

		/// <summary>
		/// Gets the server this channel relates to.
		/// </summary>
		property IRCServer^ Server
        {
            IRCServer^ get()			{ return m_server;  }
			void set(IRCServer^ value)  { m_server = value; }
        }

		/// <summary>
		/// Gets the total number of messages recieved.
		/// </summary>
		property unsigned int MessagesRecieved
		{
            unsigned int get()				{ return m_messagesRecieved;  }
		}

		/// <summary>
		/// Gets the name of this channel.
		/// </summary>
		property Platform::String^ Name
        {
            Platform::String^ get()	{ return m_name; }
            void set(Platform::String^ value)	{ m_name = value; }
        }

		/// <summary>
		/// Gets the type of this channel.
		/// </summary>
		property IRCChannelType Type
        {
			IRCChannelType get()	{ return m_channelType; }
        }

		/// <summary>
		/// Gets or sets if we are currently parsing this channels user list.
		/// </summary>
		property bool ParsingUserList
        {
			bool get()	{ return m_parsingUserList; }
			void set(bool value) { m_parsingUserList = value; }
        }
				
		/// <summary>
		/// Gets the number of messages recieved since the last call to ResetNewMessageCount();
		/// </summary>
		property unsigned int NewMessageCount
		{
			unsigned int get()				{ return m_messagesRecieved - m_lastSeenMessageCount; }
		}

		/// <summary>
		/// Gets the list of messages held by this channel.
		/// </summary>
		property Windows::Foundation::Collections::IVector<IRCMessage^>^ Messages
        {
			Windows::Foundation::Collections::IVector<IRCMessage^>^ get() { return m_messages; }
        }

		// ------------------------------------------------------------------------------------
		// Public methods.
		// ------------------------------------------------------------------------------------

		IRCChannel								(IRCServer^ server, Platform::String^ name, IRCChannelType type);
		virtual ~IRCChannel						();

		void	 ResetNewMessageCount			();

		void	 AddMessage						(IRCMessage^ message);
		void	 AsyncLeave						();
		
		void	 AsyncOpUser					(IRCUser^ user);
		void	 AsyncVoiceUser					(IRCUser^ user);
		void	 AsyncDePrivilageUser			(IRCUser^ user);
		void	 AsyncKickUser					(IRCUser^ user);
		void	 AsyncBanUser					(IRCUser^ user);

	};

}