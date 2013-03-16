// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include <pch.h>
#include "IRCChannel.h"

namespace LibIRC
{
	// Forward references.
	ref class IRCChannel;
	ref class IRCServer;
	
	/// <summary>
	/// Stores information on a users channel mode.
	/// <summary>
	public ref class IRCUserChannelMode sealed
	{
	private:
		IRCChannel^			m_channel;
		Platform::String^	m_mode;

	public:

		/// <summary>
		/// Gets the channel this mode refers to.
		/// </summary>
		property IRCChannel^ Channel
		{
			IRCChannel^ get()			{ return m_channel; }
			void set(IRCChannel^ value) { m_channel = value; }
		}
		
		/// <summary>
		/// Gets a string of mode flags (+b/+s, etc) for this user channel mode.
		/// </summary>
		property Platform::String^ Mode
		{
			Platform::String^ get()			  { return m_mode; }
			void set(Platform::String^ value) { m_mode = value; }
		}

		IRCUserChannelMode(IRCChannel^ channel, Platform::String^ mode);
	};

	/// <summary>
	/// The IRC channel class manages code access to a specific channel on a given IRCServer.
	///	Contains all the code allowing the user to do any channel specific operations.
	/// <summary>
	public ref class IRCUser sealed
	{
	private:
		IRCServer^			m_server;
		Platform::String^	m_name;
		Platform::String^	m_sortName;
		Platform::String^	m_host;
		Platform::String^	m_nickName;
		
		Platform::String^	m_mode;
		
		Platform::Collections::Vector<IRCChannel^>^ m_activeChannels;
		Platform::Collections::Vector<IRCUserChannelMode^>^ m_userChannelModes;

	public:

		// ------------------------------------------------------------------------------------
		// Public properties.
		// ------------------------------------------------------------------------------------

		/// <summary>
		/// Gets the server this channel relates to.
		/// </summary>
		property IRCServer^ Server
        {
            IRCServer^ get()	{ return m_server; }
        }

		/// <summary>
		/// Gets the name of this user.
		/// </summary>
		property Platform::String^ Name
        {
            Platform::String^ get()	{ return m_name; }
        }
		
		/// <summary>
		/// Gets or sets the sort name of this user. This is used internally to sort lists of users accurately.
		/// </summary>
		property Platform::String^ SortName
        {
            Platform::String^ get()	{ return m_sortName; }
            void set(Platform::String^ value)	{ m_sortName = value; }
        }

		/// <summary>
		/// Gets the host of this user.
		/// </summary>
		property Platform::String^ Host
        {
            Platform::String^ get()	{ return m_host; }
        }

		/// <summary>
		/// Gets the nickname of this user.
		/// </summary>
		property Platform::String^ NickName
        {
            Platform::String^ get()	{ return m_nickName; }
            void set(Platform::String^ value)	{ m_nickName = value; }
        }

		/// <summary>
		/// Gets the mode of this user.
		/// </summary>
		property Platform::String^ Mode
        {
            Platform::String^ get()	{ return m_mode; }
            void set(Platform::String^ value)	{ m_mode = value; }
        }
								
		/// <summary>
		/// Gets the list of servers held by this controller.
		/// </summary>
		property Windows::Foundation::Collections::IVector<IRCChannel^>^ ActiveChannels
        {
			Windows::Foundation::Collections::IVector<IRCChannel^>^ get() { return m_activeChannels; }
        }

		// ------------------------------------------------------------------------------------
		// Public methods.
		// ------------------------------------------------------------------------------------
		IRCUser								(IRCServer^ server, Platform::String^ name, Platform::String^ host, Platform::String^ nickname);
		virtual ~IRCUser					();

		void				AddActiveChannel			(IRCChannel^ channel);
		void				RemoveActiveChannel			(IRCChannel^ channel);

		void				SetChannelMode				(IRCChannel^ channel, Platform::String^ mode);
		Platform::String^ 	GetChannelMode				(IRCChannel^ channel);
		bool				IsChannelModeSet			(IRCChannel^ channel, wchar_t mode_chr);

		void				SetMode						(Platform::String^ mode);
		Platform::String^ 	GetMode						();
		bool				IsModeSet					(wchar_t mode_chr);

		Platform::String^	GetFriendlyTextForChannel	(IRCChannel^ channel);
		bool				IsActiveInChannel			(IRCChannel^ channel);

	};

}