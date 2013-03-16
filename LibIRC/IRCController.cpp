// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "IRCController.h"

using namespace LibIRC;
using namespace Platform;
using namespace Collections;
using namespace Windows::Foundation::Collections;

IRCController::IRCController()
{
	m_servers = ref new Vector<IRCServer^>();
}

IRCController::~IRCController()
{
	m_servers->Clear();
	m_servers = nullptr;
}

/// <summary>
/// Creates the given server and assigns this as its controller.
/// </summary>
IRCServer^ IRCController::CreateServer(String^ hostname, String^ port)
{
	IRCServer^ server = ref new IRCServer(this, hostname, port);
	m_servers->Append(server);
	return server;
}

/// <summary>
/// Removes the given server from this controller.
/// </summary>
void IRCController::RemoveServer(IRCServer^ server)
{
	server->AsyncDisconnect();

	unsigned int index;
	m_servers->IndexOf(server, &index);
	m_servers->RemoveAt(index);
}

/// <summary>
/// Gets a list of all servers this controller is looking after.
/// </summary>
IVector<IRCServer^>^ IRCController::GetServerList()
{
	return m_servers;
}