// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include <pch.h>
#include "IRCServer.h"

namespace LibIRC
{
	// Forward references.
	ref class IRCServer;
	
	/// <summary>
	/// The IRC Controller is the base interface used to access any IRC
	/// related code. The user who wants to perform IRC operations just
	/// needs to create an instance of this class and call the 
	/// appropriate methods.
	/// <summary>
	public ref class IRCController sealed
	{
	private:
		Platform::Collections::Vector<IRCServer^>^	m_servers;

	public:
		
		/// <summary>
		/// Gets the list of servers held by this controller.
		/// </summary>
		property Windows::Foundation::Collections::IVector<IRCServer^>^ Servers
        {
			Windows::Foundation::Collections::IVector<IRCServer^>^ get() { return m_servers; }
        }

		// ------------------------------------------------------------------------------------
		// Public methods.
		// ------------------------------------------------------------------------------------
		IRCController																();
		virtual ~IRCController														();

		IRCServer^													CreateServer	(Platform::String^ hostname, Platform::String^ port);
		void														RemoveServer	(IRCServer^ server);
		Windows::Foundation::Collections::IVector<IRCServer^>^		GetServerList	();

	};

}