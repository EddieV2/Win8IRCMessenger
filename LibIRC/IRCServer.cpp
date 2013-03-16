// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "IRCServer.h"
#include "IRCCommand.h"
#include "IRCChannel.h"
#include "IRCMessage.h"
#include "IRCUser.h"

using namespace Platform;
using namespace Concurrency;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;
using namespace std;
using namespace LibIRC;

IRCServer::IRCServer(IRCController^ controller, String^ hostname, String^ port)
{
	m_controller		= controller;
	m_name				= "Untitled Server";
	m_hostname			= hostname;

	try
	{
		m_network_hostname	= ref new HostName(m_hostname);
	}
	catch (Exception^ ex)
	{
		m_network_hostname	= ref new HostName("irc.example.com");
	}
		
	m_port				= port;
	m_socket			= ref new StreamSocket();
	m_connected			= false;
	m_input_buffer		= "";
	m_username			= "UnnamedUser" + (rand() % 100000);
	m_password			= "";
	m_userHostname		= "";
	m_userRealName		= "";	
	m_channels			= ref new Collections::Vector<IRCChannel^>();
	m_users				= ref new Collections::Vector<IRCUser^>();
	m_usersMap			= ref new Collections::Map<String^, IRCUser^>();
	m_connecting		= false;
	m_disconnecting		= false;
	m_away				= false;
	m_awayMessage		= "";
	m_active			= false;

	m_currentNickname   = m_username;

	// A fake "server" channel that we use to log all the server chatter (MOTD, etc).
	IRCChannel^ server_channel = ref new IRCChannel(this, "Server", IRCChannelType::IRC_CHANNEL_TYPE_DUMMY);
	m_channels->Append(server_channel);
}

IRCServer::~IRCServer()
{
	m_controller		= nullptr;
	m_name				= nullptr;
	m_hostname			= nullptr;
	m_port				= nullptr;
	m_network_hostname	= nullptr;
	m_connected			= false;
	m_input_buffer		= nullptr;
	m_username			= nullptr;
	m_password			= nullptr;
	m_userHostname		= nullptr;
	m_userRealName		= nullptr;
	m_connecting		= false;
	m_disconnecting		= false;
	m_currentNickname	= nullptr;
	m_away				= false;
	m_awayMessage		= nullptr;

	m_channels->Clear();
	m_channels			= nullptr;

	m_users->Clear();
	m_users				= nullptr;
	
	m_usersMap->Clear();
	m_usersMap			= nullptr;

	// Close stream.
	CloseStream(false);
}

/// <summary>
/// Closes the stream to the server.
/// </summary>
void IRCServer::CloseStream(bool byError)
{
	if (byError == true && m_disconnecting == true)
	{
		byError = false;
	}
	m_disconnecting		= false;

	if (m_connected == false)
	{
		return;
	}

	m_connected			= false;
	m_connecting		= false;

	bool disconnect = (m_socket != nullptr);
	RecreateSocket();

	if (disconnect = true)
	{
		// Notify any listening users that the socket was disconnected.
		OnDisconnect(this);
	}

	// If our stream got closed by an error, then attempt to reconnect.
	if (byError == true)
	{	
		AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, "Disconnected, retrying in " + IRC_SERVER_CONNECT_RETRY_INTERVAL_START + "ms ..."));
		AsyncConnect(true, IRC_SERVER_CONNECT_RETRY_INTERVAL_START);
	}
}

/// <summary>
/// Recreates the network socket for use after being closed or disposed of.
/// </summary>
void IRCServer::RecreateSocket()
{
	// Dispose of input reader.
	if (m_input_reader != nullptr)
	{
		try
		{
			m_input_reader->DetachStream();
			m_input_reader->DetachBuffer();
		}
		catch (Exception^ ex)
		{
			// Ignore, we're dumping it anyway.
		}
		m_input_reader = nullptr;
	}
	
	// Dispose of input reader.
	if (m_output_writer != nullptr)
	{
		try
		{
			m_output_writer->DetachStream();
			m_output_writer->DetachBuffer();
		}
		catch (Exception^ ex)
		{
			// Ignore, we're dumping it anyway.
		}
		m_output_writer = nullptr;
	}

	// This seems really counter-intuative to have to "delete" sockets to close them,
	// I wonder why there is not explicit close method anymore. This is the correct
	// way to do it though, according to the docs;
	// http://msdn.microsoft.com/en-us/library/windows/apps/windows.networking.sockets.streamsocket.close.aspx

	if (m_socket != nullptr)
	{
		delete m_socket;
		m_socket = nullptr;
		
		// Create a new socket.
		m_socket = ref new StreamSocket();
	}
}

/// <summary>
/// Asyncronously connects to the server using default connection settings.
/// </summary>
void IRCServer::AsyncConnect()
{
	AsyncConnect(true, IRC_SERVER_CONNECT_RETRY_INTERVAL_START);
}

/// <summary>
/// Asyncronously connects to the server.
/// </summary>
void IRCServer::AsyncConnect(bool keepRetrying, int retryInterval)
{
	if (m_connected == true)
	{
		ForceDisconnect();
	}
	if (m_connecting == true)
	{
		return;
	}
	
	m_active = true;

	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, "Connecting to " + m_hostname + ":" + m_port + " ..."));
			
	// Recreate the socket.
	RecreateSocket();

	m_connecting = true;

	task<void>(m_socket->ConnectAsync(m_network_hostname, m_port)).then([this, keepRetrying, retryInterval] (task<void> connectTask) -> bool
    {
        try
        {
            // Check we have connected successfully.
			connectTask.get();
			m_connected  = true;
			m_connecting = false;

			// Create sending stuff.
			if (m_output_writer != nullptr)
			{
				m_output_writer->DetachStream();
				m_output_writer->DetachBuffer();
				m_output_writer = nullptr;
			}
			m_output_writer = ref new DataWriter(m_socket->OutputStream);

			// Invoke the delegate.
			OnConnectSuccess(this);

			// Send default login.

			if (m_password != "" && m_password != "")
			{
				SendMessageLine("PASS " + m_password);
			}

			String^ userhostname = m_userHostname;
			String^ userrealname = m_userRealName;
			if (userhostname == "")
			{
				userhostname = "*";
			}
			if (userrealname == "")
			{
				userrealname = m_username;
			}
			SendMessageLine("USER " + m_username + " " + userhostname + " " + m_hostname + " :" + userrealname);
			SendMessageLine("NICK " + m_username);

			m_currentNickname = m_username;
			
			// Begin reading data.
			BeginReadLoop();
			
			return true;
        }
        catch (Exception^ exception)
        {
			m_connected  = false;

			return false;
        }

    }).then([this, keepRetrying, retryInterval] (task<bool> previousTask)
	{
		bool rerun = false;
		
		// Try and get the result of the connection.
        try
        {
			bool val = previousTask.get();
			if (val == false)	
			{
				rerun = true;
			}
        }
        catch (Exception^ exception)
        {
			rerun = true;
        }			

		// Shall we attempt to retry the connection?
		if (keepRetrying == true && rerun == true)
		{
			// Connection failed :(
			OnConnectFailure(this);

			// Use exponential backoff to prevent unneccessarily wasting 
			// CPU / Network resources.
			int newDelay = retryInterval * IRC_SERVER_CONNECT_RETRY_INTERVAL_STEP;
			newDelay = max(min(newDelay, IRC_SERVER_CONNECT_RETRY_INTERVAL_END), 0);
			
			AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, "Disconnected, retrying in " + newDelay + "ms ..."));
			//OutputDebugString(("Retrying connection again in "+newDelay+"ms ...\n")->Data());
		
			// Event to be throw when timer fires.
			task_completion_event<void> complete_event;

			// Create one-shot timer.
			Concurrency::timer<int>* fire_once = new timer<int>(newDelay, 0, nullptr, false);

			// Create a callback.
			auto callback = new call<int>([complete_event](int)
			{
				complete_event.set();						
			});

			// Link everything up.
			fire_once->link_target(callback);
			fire_once->start();

			// Wait until we are done!
			task<void>(complete_event).then([this, callback, fire_once, newDelay]()
			{
				delete callback;
				delete fire_once;

				AsyncConnect(true, newDelay);
			});
		}
		else
		{
			m_connecting = false;
			if (rerun == true)
			{
				OnConnectFailure(this);
			}
		}

	});
}

/// <summary>
/// Begins the main reading loop when accepting data from the server.
/// </summary>
void IRCServer::BeginReadLoop()
{
	// Create our data reader.
	if (m_input_reader != nullptr)
	{
		m_input_reader->DetachStream();
		m_input_reader->DetachBuffer();
		m_input_reader = nullptr;
	}
	m_input_reader = ref new DataReader(m_socket->InputStream);
	m_input_reader->InputStreamOptions = InputStreamOptions::Partial;

	m_input_buffer = "";

	ReadLoop();
}

/// <summary>
/// Sits and reads data from the server, until disconnected.
/// </summary>
void IRCServer::ReadLoop()
{
	if (m_input_reader == nullptr)
	{
		return;
	}

	try
	{

		// Begin reading data asyncronously.
		task<unsigned int> loadedSize(m_input_reader->LoadAsync(128));
		loadedSize.then([this] (unsigned int size) -> unsigned int
		{		
			if (m_input_reader == nullptr)
			{
				return 0;
			}

			try
			{
				// Attempt to read and process the data.
				if (size > 0)
				{
					// InputStream::ReadString currently has a bug where it will output errors when attempting to read unicode.
					// http://msdn.microsoft.com/en-US/library/windows/apps/windows.storage.streams.datareader.readstring
					//String^ data = m_input_reader->(size);
					
					// So we read it as ASCII bytes instead.
					String^ data = "";
					for (unsigned int i = 0; i < size; i++)
					{
						wchar_t chr = m_input_reader->ReadByte();
						data += chr;
					}

					ProcessInputData(data);
				}
			}
			catch (Exception^ ex)
			{
				return 0;
			}

			return size;

		}).then([this] (task<unsigned int> previousTask)
		{
			try
			{
				// If all went well, then start the reading loop again!
				int size = previousTask.get();
				if (size > 0)
				{
					ReadLoop();
				}
				else
				{
					CloseStream(true);
				}
			}
			catch (Exception^ ex2)
			{
				CloseStream(true);
			}
		});

	}
	catch (Exception^ ex)
	{
		CloseStream(true);
	}
}

/// <summary>
/// Proceses the give input data from the server, and dispatches it to be parsed when a full line is recieved.
/// </summary>
void IRCServer::ProcessInputData(String^ data)
{
	String^ partialLine = "";

	// Add new data onto full buffer.
	m_input_buffer += data;

	// Try and extract lines from buffer.
	for (unsigned int i = 0; i < m_input_buffer->Length(); i++)
	{
		const wchar_t chr = *(m_input_buffer->Data() + i);

		if (chr == '\n' || chr == '\r' || chr == '\0')
		{
			if (partialLine != "")
			{
				ParseInputLine(partialLine);
			}
			partialLine = "";
		}
		else
		{
			partialLine += chr;
		}
	}

	// Store the remaining partial line in the buffer.
	m_input_buffer = partialLine;
}

/// <summary>
/// Parses the given line of input form the server.
/// </summary>
void IRCServer::ParseInputLine(String^ data)
{
	OutputDebugString(L"<< ");
	OutputDebugString(data->Data());
	OutputDebugString(L"\n");

	// Strip out mIRC non-standard color cods.
	data = StripColorCodes(data);

	IRCCommand^ command = ref new IRCCommand(data);
	ParseCommand(command);
}

/// <summary>
/// Used in conjunction with StripColorCodes to correctly parse out color codes.
/// </summary>
bool IRCServer::EatColorCodeChar(Platform::String^ data, wchar_t min, wchar_t max, unsigned int& pos)
{
	if (pos + 1 >= data->Length())
	{
		return false;
	}

	wchar_t chr = data->Data()[pos + 1];

	if (chr >= min && chr <= max)
	{
		pos++;
		return true;
	}

	return false;
}

/// <summary>
/// Strips out all propriatory color codes used by non-standard clients.
/// </summary>
Platform::String^ IRCServer::StripColorCodes(Platform::String^ data)
{
	Platform::String^ result = "";

	for (unsigned int i = 0; i < data->Length(); i++)
	{
		wchar_t chr = data->Data()[i];

		// Color code.
		// Formatting codes.
		if (chr < 32 && chr != 1) // Ignore CTCP chars.
		{
			if (EatColorCodeChar(data, '0', '9', i))
			{
				EatColorCodeChar(data, '0', '9', i);
				
				if (EatColorCodeChar(data, ',', ',', i))
				{
					if (EatColorCodeChar(data, '0', '9', i))
					{
						EatColorCodeChar(data, '0', '9', i);
					}
				}
			}

		}
		else
		{
			result += chr;
		}
	}

	return result;
}

/// <summary>
/// Sends a raw string command to the server.
/// </summary>
void IRCServer::SendMessageLine(Platform::String^ data)
{
	if (m_connected == false)
	{
		// Abort abort!
		return;
	}
	
	OutputDebugString(L">> ");
	OutputDebugString(data->Data());
	OutputDebugString(L"\n");
	
	try
	{
		m_output_writer->WriteString(data + "\n");

		task<unsigned int> (m_output_writer->StoreAsync()).then([this] (task<unsigned int> writeTask)
		{
			try
			{
				writeTask.get();
				m_socket->OutputStream->FlushAsync();
			}
			catch (Exception^ ex)
			{
				CloseStream(true);
			}
		});
	}
	catch (Exception^ ex)
	{
		CloseStream(true);
		return;
	}
}

/// <summary>
/// Sends a command to the server.
/// </summary>
void IRCServer::SendCommand(IRCCommand^ data)
{
	SendMessageLine(data->FormatAsString());
}

/// <summary>
/// Sends a QUIT message to asyncronously disconnect from the server.
/// </summary>
void IRCServer::AsyncDisconnect()
{
	m_disconnecting	= true;	
	m_active = false;
	SendMessageLine("QUIT");	
}

/// <summary>
/// Forcefully and immediately disconnects the user from this server.
/// </summary>
void IRCServer::ForceDisconnect()
{
	m_active = false;
	CloseStream(false);
}

/// <summary>
/// Adds a given message to this server, and routes it to the correct channel (or server window if non specified).
/// </summary>
void IRCServer::AddMessage(IRCMessage^ msg)
{
	// Null origin means message goes to all channels.
	if (msg->Origin == nullptr)
	{
		for (unsigned int i = 0; i < m_channels->Size; i++)
		{
			IRCChannel^ channel = m_channels->GetAt(i);
			channel->AddMessage(msg);

			msg->OriginInternal = channel;
			OnMessageRecieved(this, msg);
		}

		msg->OriginInternal = nullptr;
	}
	else
	{
		msg->Origin->AddMessage(msg);
		OnMessageRecieved(this, msg);
	}
}

/// <summary>
/// Little sorting function to make sure users are always in alphabetical order.
/// </summary>
bool IRCUserSort(IRCUser^ u1, IRCUser^ u2)
{
	return u1->SortName < u2->SortName;
}

/// <summary>
/// Adds a user to the user list and sorts it alphabetically.
/// </summary>
void IRCServer::AddUser(IRCUser^ user)
{
	AddUserNoSorting(user);
	SortUsers();
}

/// <summary>
/// Adds a user to the user list without sorting the list (useful for adding a lot of users en-mass quickly).
/// </summary>
void IRCServer::AddUserNoSorting(IRCUser^ user)
{
	m_users->Append(user);
	m_usersMap->Insert(StringHelper::ToUpperCase(user->NickName), user);
}

/// <summary>
/// Sorts user list alphabetically.
/// </summary>
void IRCServer::SortUsers()
{
	for (unsigned int i = 0; i < m_users->Size; i++)
	{
		IRCUser^ user = m_users->GetAt(i);
		user->SortName = StringHelper::ToUpperCase(user->Name);
	}
    std::sort(begin(m_users), end(m_users), IRCUserSort);
}

/// <summary>
/// Gets a given IRCChannel representation of a channel based on their in-irc name.
/// </summary>
IRCChannel^ IRCServer::GetChannelByName(Platform::String^ name)
{
	for (unsigned int i = 0; i < m_channels->Size; i++)
	{
		IRCChannel^ channel = m_channels->GetAt(i);
		if (StringHelper::ToUpperCase(channel->Name) == StringHelper::ToUpperCase(name))
		{
			return channel;
		}
	}

	return nullptr;
}

/// <summary>
/// Gets a given IRCUser representation of a user based on their in-irc name.
/// </summary>
IRCUser^ IRCServer::GetUserByName(Platform::String^ name)
{
	// Strip symbols.
	Platform::String^ originalName = name;
	name = "";

	for (unsigned int j = 0; j < originalName->Length(); j++)
	{
		wchar_t chr = originalName->Data()[j];

		if (name != "")
		{
			name += chr;
		}
		else
		{
			if (chr == '+' || chr == '@' || chr == '~' || chr == '&' || chr == '%')
			{
				// Skip mode.
			}
			else
			{
				name += chr;
			}
		}
	}

	// Make it uppercase.
	name = StringHelper::ToUpperCase(name);

	// Look in our map.
	if (m_usersMap->HasKey(name))
	{
		IRCUser^ user = m_usersMap->Lookup(name);
		return user;
	}
	else
	{
		return nullptr;
	}
}

/// <summary>
/// Asks to join the given channel using the given password - asyncronously executes.
/// </summary>
void IRCServer::AsyncJoinChannel(String^ name, String^ password)
{
	if (password != "")
	{
		SendCommand(IRCCommand::Create("JOIN", name, password));
	}
	else
	{
		SendCommand(IRCCommand::Create("JOIN", name));
	}
}

/// <summary>
/// Sends a query user command - basically opens up a "private chat" channel between the two users.
/// </summary>
IRCChannel^ IRCServer::QueryUser(IRCUser^ user)
{
	// Find previous channel we are chatting to the user on.
	IRCChannel^ channel = GetChannelByName(user->NickName);
	bool newChannel = false;

	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, user->NickName, IRCChannelType::IRC_CHANNEL_TYPE_USER);
		m_channels->Append(channel);
		newChannel = true;
	}

	// Get our user.
	IRCUser^ ouruser = GetUserByName(m_currentNickname);
	if (ouruser == nullptr)
	{
		ouruser = ref new IRCUser(this, m_currentNickname, "", m_currentNickname);
		AddUser(ouruser);
	}

	// Did we just join this?
	if (newChannel == true)
	{
		user->AddActiveChannel(channel);
		ouruser->AddActiveChannel(channel);

		OnJoinChannel(this, channel, ouruser);
	}

	return channel;
}

/// <summary>
/// Disposes of a given channel and forces the user to leave.
/// </summary>
void IRCServer::DisposeChannel(IRCChannel^ channel, bool invokeLeave)
{
	unsigned int index = 0;

	// Remove anything important from channel.
	m_channels->IndexOf(channel, &index);
	m_channels->RemoveAt(index);
	channel->Server = nullptr;
	channel->Messages->Clear();

	// Remove all users from the channel.
	for (unsigned int i = 0; i < m_users->Size; i++)
	{
		IRCUser^ user = m_users->GetAt(i);		
		user->RemoveActiveChannel(channel);
	}

	// Invoke leave?
	if (invokeLeave == true)
	{
		// Find our user.
		IRCUser^ user = GetUserByName(m_currentNickname);
	
		// Tell everyone a user left this channel.
		OnLeaveChannel(this, channel, user);
	}
}

/// <summary>
/// Attempts to parse the message the user inputs and send the appropriate
/// information to the server.
/// </summary>
void IRCServer::ParseAndSendMessage(IRCChannel^ destination, Platform::String^ message)
{
	// Why would you send an empty message ;_;
	if (message->Length() == 0)
	{
		return;
	}

	// Disconnected?
	if (m_connected == false)
	{
		AddMessage(ref new IRCMessage(destination, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, "Cannot send messages: Currently disconnected from server.", nullptr));
		return;
	}

	// Table that defines where we dispatch different
	// "special" commands to, and what their parameter counts are.
	static IRCCommandHandler* CommandDispatchTable[] = {

		   //									PARAMS
		   //					 COMMAND        MIN MAX FUNCTION					SYNTAX
		   new IRCCommandHandler("ACTION",		1,	1, &IRCServer::SendAction,		"/action <message>"),
		   new IRCCommandHandler("ME",			1,	1, &IRCServer::SendAction,		"/me <message>"),
		   new IRCCommandHandler("NICK",		1,	1, &IRCServer::SendNick,		"/nick <new-nickname>"),
		   new IRCCommandHandler("MSG",			2,	2, &IRCServer::SendMsg,			"/msg <user> <message>"),
		   new IRCCommandHandler("PRIVMSG",		2,	2, &IRCServer::SendMsg,			"/privmsg <user> <message>"),
		   new IRCCommandHandler("OPER",		2,	2, &IRCServer::SendOper,		"/oper <user> <password>"),
		   new IRCCommandHandler("QUIT",		0,	1, &IRCServer::SendQuit,		"/quit [message]"),
		   new IRCCommandHandler("HELP",		0,	0, &IRCServer::SendHelp,		"/help"),
		   new IRCCommandHandler("MOTD",		0,	0, &IRCServer::SendMOTD,		"/motd"),
		   new IRCCommandHandler("LIST",		0,	0, &IRCServer::SendList,		"/list"),
		   new IRCCommandHandler("CHANSERV",	1,	1, &IRCServer::SendChanServ,	"/chanserv <message>"),
		   new IRCCommandHandler("NICKSERV",	1,	1, &IRCServer::SendNickServ,	"/nickserv <message>"),
		   new IRCCommandHandler("MEMOSERV",	1,	1, &IRCServer::SendMemoServ,	"/memoserv <message>"),
		   new IRCCommandHandler("AWAY",		0,	1, &IRCServer::SendAway,		"/away [message]"),
		   new IRCCommandHandler("BACK",		0,	0, &IRCServer::SendBack,		"/back"),
		   new IRCCommandHandler("CTCP",		2,	2, &IRCServer::SendCTCP,		"/ctcp <nickname> <query>"),
		   new IRCCommandHandler("INVITE",		2,	2, &IRCServer::SendInvite,		"/invite <nickname> <channel-name>"),
		   new IRCCommandHandler("KNOCK",		1,	2, &IRCServer::SendKnock,		"/knock <channel-name> [message]"),
		   new IRCCommandHandler("MODE",		2,	3, &IRCServer::SendMode,		"/mode <channel-name> <mode> [arguments]"),
		   new IRCCommandHandler("JOIN",		1,	1, &IRCServer::SendJoin,		"/join <channel-name>"),
		   new IRCCommandHandler("PART",		1,	2, &IRCServer::SendPart,		"/part <channel-name> [message]"),
		   new IRCCommandHandler("PARTALL",		0,	1, &IRCServer::SendPartAll,		"/partall [message]"),
		   new IRCCommandHandler("TOPIC",		1,	2, &IRCServer::SendTopic,		"/topic <channel-name> [topic]"),
		   new IRCCommandHandler("RAW",			1,	2, &IRCServer::SendRaw,			"/raw <command> [arguments]"),
		   new IRCCommandHandler("KICK",		1,	3, &IRCServer::SendKick,		"/kick <channel-name> <nick> [reason]"),
		   new IRCCommandHandler("BAN",			1,	3, &IRCServer::SendBan,			"/ban <channel-name> <nick> "),
		   new IRCCommandHandler("UNBAN",		1,	3, &IRCServer::SendBan,			"/unban <channel-name> <nick> "),
		   new IRCCommandHandler("VERSION",		0,	0, &IRCServer::SendVersion,		"/version"),
		   NULL,

	};

	// Is this a command we are parsing?
	if (message->Data()[0] == '/')
	{
		Platform::String^									command				= "";
		Platform::String^									partialParameter	= "";
		Platform::Collections::Vector<Platform::String^>^	parameters			= ref new Platform::Collections::Vector<Platform::String^>();

		// Nothing else given?
		if (message->Length() < 2)
		{
			AddMessage(ref new IRCMessage(destination, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, "No command given.", nullptr));
			return;
		}

		// Parse out the command first..
		unsigned int i = 1;		
		for (i = 1; i < message->Length() ; i++)
		{
			wchar_t chr = message->Data()[i];
			if (chr == ' ')
			{
				i++;
				break;
			}
			else
			{
				command += chr;
			}
		}

		// See if its a valid command.
		unsigned int index = 0;
		IRCCommandHandler* commandHandler = NULL;
		while (true)
		{
			IRCCommandHandler* handler = CommandDispatchTable[index];
			if (handler == NULL)
			{
				break;
			}
			else
			{
				if (StringHelper::ToUpperCase(handler->Name) == StringHelper::ToUpperCase(command))
				{
					commandHandler = handler;
					break;
				}
			}
			index++;
		}

		if (commandHandler == NULL)
		{
			AddMessage(ref new IRCMessage(destination, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, "Unknown command: /" + command, nullptr));
			return;
		}

		// Parse all the parameters.
		for (; i < message->Length(); i++)
		{
			wchar_t chr = message->Data()[i];
			if (chr == ' ' && parameters->Size < commandHandler->ParameterMax - 1)
			{
				parameters->Append(partialParameter);
				partialParameter = "";
			}
			else
			{
				partialParameter += chr;
			}
		}
		
		// Add final parameter to list.
		if (partialParameter != "")
		{
			parameters->Append(partialParameter);
			partialParameter = "";
		}

		// Check we have the correct number of parameters.
		if (parameters->Size >= commandHandler->ParameterMin && parameters->Size <= commandHandler->ParameterMax)
		{
			(ref new CommandHandlerDelegate(this, commandHandler->Function))->Invoke(destination, parameters);
		}
		else
		{	
			AddMessage(ref new IRCMessage(destination, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, "Command Syntax: " + commandHandler->Syntax, nullptr));
		}
	}
	else
	{
		SendMessageToChannel(destination, message);
	}
}

/// <summary>
///	  Command: /me /action
/// Parameters: <message>
/// </summary>
void IRCServer::SendAction(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	if (destination->Type == IRCChannelType::IRC_CHANNEL_TYPE_DUMMY)
	{
		AddMessage(ref new IRCMessage(GetChannelByName("Server"), nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, "You can't send messages to this channel.", nullptr));
		return;
	}

	Platform::String^ message = parameters->GetAt(0);
	message = wchar_t(1) + "ACTION " + message + wchar_t(1);

	SendMessageToChannel(destination, message);
}

/// <summary>
///	  Command: /nick
/// Parameters: <new-nickname>
/// </summary>
void IRCServer::SendNick(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ newNickname = parameters->GetAt(0);
	SendCommand(IRCCommand::Create("NICK", newNickname));
}

/// <summary>
///	  Command: /msg
/// Parameters: <user> <message>
/// </summary>
void IRCServer::SendMsg(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ user = parameters->GetAt(0);
	Platform::String^ message = parameters->GetAt(1);
	SendCommand(IRCCommand::Create("PRIVMSG", user, message));
}

/// <summary>
///	  Command: /oper
/// Parameters: <user> <password> 
/// </summary>
void IRCServer::SendOper(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ user = parameters->GetAt(0);
	Platform::String^ password = parameters->GetAt(1);
	SendCommand(IRCCommand::Create("OPER", user, password));
}

/// <summary>
///	  Command: /quit
/// Parameters: [message] 
/// </summary>
void IRCServer::SendQuit(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	m_disconnecting = true;
	if (parameters->Size > 0)
	{
		SendCommand(IRCCommand::Create("QUIT", parameters->GetAt(0)));
	}
	else
	{
		SendCommand(IRCCommand::Create("QUIT"));
	}
}

/// <summary>
///	  Command: /help
/// Parameters: 
/// </summary>
void IRCServer::SendHelp(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	SendCommand(IRCCommand::Create("HELP"));
}

/// <summary>
///	  Command: /motd
/// Parameters: 
/// </summary>
void IRCServer::SendMOTD(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	SendCommand(IRCCommand::Create("MOTD"));
}

/// <summary>
///	  Command: /list
/// Parameters: 
/// </summary>
void IRCServer::SendList(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	SendCommand(IRCCommand::Create("LIST"));
}

/// <summary>
///	  Command: /chanserv
/// Parameters: <message>
/// </summary>
void IRCServer::SendChanServ(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ message = parameters->GetAt(0);
	SendCommand(IRCCommand::Create("PRIVMSG", "chanserv", message));
}

/// <summary>
///	  Command: /nickserv
/// Parameters: <message>
/// </summary>
void IRCServer::SendNickServ(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ message = parameters->GetAt(0);
	SendCommand(IRCCommand::Create("PRIVMSG", "nickserv", message));
}

/// <summary>
///	  Command: /memoserv
/// Parameters: <message>
/// </summary>
void IRCServer::SendMemoServ(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ message = parameters->GetAt(0);
	SendCommand(IRCCommand::Create("PRIVMSG", "memoserv", message));
}

/// <summary>
///	  Command: /away
/// Parameters: [message]
/// </summary>
void IRCServer::SendAway(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	m_away = true;
	m_awayMessage = parameters->Size > 0 ? parameters->GetAt(0) : "";
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, "You are now set as away.", nullptr));		
}

/// <summary>
///	  Command: /back
/// Parameters: 
/// </summary>
void IRCServer::SendBack(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	m_away = false;
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, "Your away status has been removed.", nullptr));		
}

/// <summary>
///	  Command: /ctcp
/// Parameters: <nickname> <query>
/// </summary>
void IRCServer::SendCTCP(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ nickname = parameters->GetAt(0);
	Platform::String^ query = parameters->GetAt(1);
	
	SendMessageLine("PRIVMSG " + nickname + " :" + wchar_t(1) + StringHelper::ToUpperCase(query) + wchar_t(1));
}

/// <summary>
///	  Command: /invite
/// Parameters: <nickname> <channel-name>
/// </summary>
void IRCServer::SendInvite(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ message = parameters->GetAt(0);
	Platform::String^ channel = parameters->GetAt(1);
	SendCommand(IRCCommand::Create("INVITE", message, channel));
}

/// <summary>
///	  Command: /knock
/// Parameters: <channel-name> [message]
/// </summary>
void IRCServer::SendKnock(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);

	if (parameters->Size > 1)
	{
		Platform::String^ message = parameters->GetAt(1);
		SendCommand(IRCCommand::Create("KNOCK", channel, message));
	}
	else
	{
		SendCommand(IRCCommand::Create("KNOCK", channel));	
	}
}

/// <summary>
///	  Command: /mode
/// Parameters: <channel-name> <mode> [arguments]
/// </summary>
void IRCServer::SendMode(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);
	Platform::String^ mode = parameters->GetAt(1);

	if (parameters->Size >= 3)
	{
		Platform::String^ args = parameters->GetAt(2);
		SendCommand(IRCCommand::Create("MODE", channel, mode, args));	
	}
	else
	{
		SendCommand(IRCCommand::Create("MODE", channel, mode));	
	}
}

/// <summary>
///	  Command: /join
/// Parameters: <channel-name>
/// </summary>
void IRCServer::SendJoin(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);
	SendCommand(IRCCommand::Create("JOIN", channel));	
}

/// <summary>
///	  Command: /part
/// Parameters: <channel-name> [message]
/// </summary>
void IRCServer::SendPart(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{	
	Platform::String^ channel = parameters->GetAt(0);

	if (parameters->Size > 1)
	{
		Platform::String^ message = parameters->GetAt(1);
		SendCommand(IRCCommand::Create("PART", channel, message));
	}
	else
	{
		SendCommand(IRCCommand::Create("PART", channel));	
	}
}

/// <summary>
///	  Command: /partall
/// Parameters: [message]
/// </summary>
void IRCServer::SendPartAll(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	if (parameters->Size > 0)
	{
		Platform::String^ message = parameters->GetAt(0);

		for (unsigned int i = 0; i < m_channels->Size; i++)
		{
			SendCommand(IRCCommand::Create("PART", m_channels->GetAt(i)->Name, message));
		}
	}
	else
	{
		for (unsigned int i = 0; i < m_channels->Size; i++)
		{
			SendCommand(IRCCommand::Create("PART", m_channels->GetAt(i)->Name));
		}
	}
}

/// <summary>
///	  Command: /topic
/// Parameters: <channel> [topic]
/// </summary>
void IRCServer::SendTopic(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);

	if (parameters->Size > 1)
	{
		Platform::String^ message = parameters->GetAt(1);
		SendCommand(IRCCommand::Create("TOPIC", channel, message));
	}
	else
	{
		SendCommand(IRCCommand::Create("TOPIC", channel, ""));	
	}
}

/// <summary>
///	  Command: /raw
/// Parameters: <command> [arguments]
/// </summary>
void IRCServer::SendRaw(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ command = parameters->GetAt(0);
	if (parameters->Size >= 2)
	{
		Platform::String^ args = parameters->GetAt(1);
		SendCommand(IRCCommand::Create(command, args));
	}
	else
	{
		SendCommand(IRCCommand::Create(command));
	}
}

/// <summary>
///	  Command: /kick
/// Parameters: <channel-name> <nick> [reason]
/// </summary>
void IRCServer::SendKick(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);
	Platform::String^ nickname = parameters->GetAt(1);
	if (parameters->Size >= 3)
	{
		Platform::String^ reason = parameters->GetAt(2);
		SendCommand(IRCCommand::Create("KICK", channel, nickname, reason));
	}
	else
	{
		SendCommand(IRCCommand::Create("KICK", channel, nickname));
	}
}

/// <summary>
///	  Command: /ban
/// Parameters: <channel-name> <nick> 
/// </summary>
void IRCServer::SendBan(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);
	Platform::String^ nickname = parameters->GetAt(1);
	
	SendCommand(IRCCommand::Create("MODE", channel, "+b", nickname));
}

/// <summary>
///	  Command: /unban
/// Parameters: <channel-name> <nick> 
/// </summary>
void IRCServer::SendUnBan(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	Platform::String^ channel = parameters->GetAt(0);
	Platform::String^ nickname = parameters->GetAt(1);
	
	SendCommand(IRCCommand::Create("MODE", channel, "-b", nickname));
}

/// <summary>
///	  Command: /version
/// Parameters: 
/// </summary>
void IRCServer::SendVersion(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters)
{
	SendCommand(IRCCommand::Create("VERSION"));
}

/// <summary>
///	Sends the given message to the given channel.
/// </summary>
void IRCServer::SendMessageToChannel(IRCChannel^ destination, Platform::String^ message)
{
	// Get our own user.
	IRCUser^ user = GetUserByName(m_currentNickname);
	if (user == nullptr)
	{
		IRCUser^ user = ref new IRCUser(this, m_currentNickname, "", m_currentNickname);
		AddUser(user);
	}

	switch (destination->Type)
	{			
		case IRCChannelType::IRC_CHANNEL_TYPE_NORMAL:
			AddMessage(ref new IRCMessage(destination, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, message, user));
			SendCommand(IRCCommand::Create("PRIVMSG", destination->Name, message));
			break;

		case IRCChannelType::IRC_CHANNEL_TYPE_USER:
			AddMessage(ref new IRCMessage(destination, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, message, user));
			SendCommand(IRCCommand::Create("PRIVMSG", destination->Name, message));
			break;

		case IRCChannelType::IRC_CHANNEL_TYPE_DUMMY:
			AddMessage(ref new IRCMessage(GetChannelByName("Server"), nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, "You can't send messages to this channel.", nullptr));
			break;
	}
}

/// <summary>
///	Parses the given command and dispatches it to the appropriate handler.
/// </summary>
void IRCServer::ParseCommand(IRCCommand^ cmd)
{
	// Named commands.
	IRC_PARSE_COMMAND("PING",	cmd, ParsePing);
	IRC_PARSE_COMMAND("NOTICE",	cmd, ParseNotice);
	IRC_PARSE_COMMAND("PRIVMSG",cmd, ParsePrivMsg);
	IRC_PARSE_COMMAND("ERROR",	cmd, ParseError);
	IRC_PARSE_COMMAND("PART",	cmd, ParsePart);
	IRC_PARSE_COMMAND("QUIT",	cmd, ParseQuit);
	IRC_PARSE_COMMAND("JOIN",	cmd, ParseJoin);
	IRC_PARSE_COMMAND("NICK",	cmd, ParseNick);
	IRC_PARSE_COMMAND("MODE",	cmd, ParseMode);
	IRC_PARSE_COMMAND("TOPIC",	cmd, ParseTopic);
	IRC_PARSE_COMMAND("KICK",	cmd, ParseKick);
	
	// Initial join server commands.
	IRC_PARSE_COMMAND("001",	cmd, ParseNumberReply_Welcome);
	IRC_PARSE_COMMAND("002",	cmd, ParseNumberReply_ServerVersion);
	IRC_PARSE_COMMAND("003",	cmd, ParseNumberReply_ServerDate);
	IRC_PARSE_COMMAND("004",	cmd, ParseNumberReply_ServerInfo);
	IRC_PARSE_COMMAND("005",	cmd, ParseNumberReply_ServerCommands);

	IRC_PARSE_COMMAND("251",	cmd, ParseNumberReply_UserCount);
	IRC_PARSE_COMMAND("252",	cmd, ParseNumberReply_OperatorCount);
	IRC_PARSE_COMMAND("253",	cmd, ParseNumberReply_UnknownConnections);
	IRC_PARSE_COMMAND("254",	cmd, ParseNumberReply_ChannelCount);
	IRC_PARSE_COMMAND("255",	cmd, ParseNumberReply_ConnectionCount);
	IRC_PARSE_COMMAND("265",	cmd, ParseNumberReply_LocalUserCount);
	IRC_PARSE_COMMAND("266",	cmd, ParseNumberReply_GlobalUserCount);
	IRC_PARSE_COMMAND("351",	cmd, ParseNumberReply_VersionReply);

	IRC_PARSE_COMMAND("375",	cmd, ParseNumberReply_MOTDStart);
	IRC_PARSE_COMMAND("372",	cmd, ParseNumberReply_MOTDLine);
	IRC_PARSE_COMMAND("377",	cmd, ParseNumberReply_MOTDLine);
	IRC_PARSE_COMMAND("378",	cmd, ParseNumberReply_MOTDLine);
	IRC_PARSE_COMMAND("376",	cmd, ParseNumberReply_MOTDEnd);

	// Join channel commands.
	IRC_PARSE_COMMAND("332",	cmd, ParseNumberReply_Topic);
	IRC_PARSE_COMMAND("333",	cmd, ParseNumberReply_TopicTime);
	IRC_PARSE_COMMAND("353",	cmd, ParseNumberReply_UserList);
	IRC_PARSE_COMMAND("366",	cmd, ParseNumberReply_EndOfUserList);
	IRC_PARSE_COMMAND("433",	cmd, ParseNumberReply_NickNameInUse);
	IRC_PARSE_COMMAND("221",	cmd, ParseNumberReply_UserModeIs);

	// Error replies.
	IRC_PARSE_COMMAND("401",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOSUCHNICK
	IRC_PARSE_COMMAND("402",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOSUCHSERVER
	IRC_PARSE_COMMAND("403",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOSUCHCHANNEL
	IRC_PARSE_COMMAND("404",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_CANNOTSENDTOCHAN
	IRC_PARSE_COMMAND("405",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_TOOMANYCHANNELS
	IRC_PARSE_COMMAND("406",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_WASNOSUCHNICK
	IRC_PARSE_COMMAND("407",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_TOOMANYTARGETS	
	IRC_PARSE_COMMAND("409",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOORIGIN
	IRC_PARSE_COMMAND("411",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NORECIPIENT
	IRC_PARSE_COMMAND("412",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOTEXTTOSEND
	IRC_PARSE_COMMAND("413",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOTOPLEVEL
	IRC_PARSE_COMMAND("414",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_WILDTOPLEVEL
	IRC_PARSE_COMMAND("421",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_UNKNOWNCOMMAND
	IRC_PARSE_COMMAND("422",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOMOTD
	IRC_PARSE_COMMAND("423",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOADMININFO
	IRC_PARSE_COMMAND("424",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_FILEERROR
	IRC_PARSE_COMMAND("431",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NONICKNAMEGIVEN
	IRC_PARSE_COMMAND("432",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_ERRONEUSNICKNAME
	IRC_PARSE_COMMAND("433",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NICKNAMEINUSE
	IRC_PARSE_COMMAND("436",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NICKCOLLISION
	IRC_PARSE_COMMAND("441",	cmd, ParseNumberReply_ErrorReply2Parameter); // ERR_USERNOTINCHANNEL
	IRC_PARSE_COMMAND("442",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOTONCHANNEL
	IRC_PARSE_COMMAND("443",	cmd, ParseNumberReply_ErrorReply2Parameter); // ERR_USERONCHANNEL
	IRC_PARSE_COMMAND("444",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NOLOGIN
	IRC_PARSE_COMMAND("445",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_SUMMONDISABLED
	IRC_PARSE_COMMAND("446",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_USERSDISABLED
	IRC_PARSE_COMMAND("451",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOTREGISTERED
	IRC_PARSE_COMMAND("461",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_NEEDMOREPARAMS
	IRC_PARSE_COMMAND("462",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_ALREADYREGISTRED
	IRC_PARSE_COMMAND("463",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOPERMFORHOST
	IRC_PARSE_COMMAND("464",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_PASSWDMISMATCH
	IRC_PARSE_COMMAND("465",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_YOUREBANNEDCREEP
	IRC_PARSE_COMMAND("467",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_KEYSET
	IRC_PARSE_COMMAND("471",	cmd, ParseNumberReply_ErrorCantJoinChannel); // ERR_CHANNELISFULL
	IRC_PARSE_COMMAND("472",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_UNKNOWNMODE
	IRC_PARSE_COMMAND("473",	cmd, ParseNumberReply_ErrorCantJoinChannel); // ERR_INVITEONLYCHAN
	IRC_PARSE_COMMAND("474",	cmd, ParseNumberReply_ErrorCantJoinChannel); // ERR_BANNEDFROMCHAN
	IRC_PARSE_COMMAND("475",	cmd, ParseNumberReply_ErrorCantJoinChannel); // ERR_BADCHANNELKEY
	IRC_PARSE_COMMAND("481",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOPRIVILEGES
	IRC_PARSE_COMMAND("482",	cmd, ParseNumberReply_ErrorReply1Parameter); // ERR_CHANOPRIVSNEEDED
	IRC_PARSE_COMMAND("483",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_CANTKILLSERVER
	IRC_PARSE_COMMAND("491",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_NOOPERHOST
	IRC_PARSE_COMMAND("501",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_UMODEUNKNOWNFLAG
	IRC_PARSE_COMMAND("502",	cmd, ParseNumberReply_ErrorReply0Parameter); // ERR_USERSDONTMATCH

	// Command responses.
	IRC_PARSE_COMMAND("302",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_USERHOST
	IRC_PARSE_COMMAND("303",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_ISON
	IRC_PARSE_COMMAND("301",	cmd, ParseNumberReply_AwayMessage);		// RPL_AWAY
	IRC_PARSE_COMMAND("305",	cmd, ParseNumberReply_NotAway);			// RPL_UNAWAY
	IRC_PARSE_COMMAND("306",	cmd, ParseNumberReply_NowAway);			// RPL_NOWAWAY
	IRC_PARSE_COMMAND("311",	cmd, ParseNumberReply_Reply5Parameter);	// RPL_WHOISUSER
	IRC_PARSE_COMMAND("312",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_WHOISSERVER
	IRC_PARSE_COMMAND("313",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_WHOISOPERATOR
	IRC_PARSE_COMMAND("317",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_WHOISIDLE
	IRC_PARSE_COMMAND("318",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ENDOFWHOIS
	IRC_PARSE_COMMAND("314",	cmd, ParseNumberReply_Reply5Parameter);	// RPL_WHOWASUSER
	IRC_PARSE_COMMAND("369",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ENDOFWHOWAS
	IRC_PARSE_COMMAND("321",	cmd, ParseNumberReply_ListStart);		// RPL_LISTSTART
	IRC_PARSE_COMMAND("322",	cmd, ParseNumberReply_List);			// RPL_LIST
	IRC_PARSE_COMMAND("323",	cmd, ParseNumberReply_ListEnd);			// RPL_LISTEND
	IRC_PARSE_COMMAND("324",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_CHANNELMODEIS
	IRC_PARSE_COMMAND("331",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_NOTOPIC
	IRC_PARSE_COMMAND("341",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_INVITING
	IRC_PARSE_COMMAND("342",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_SUMMONING
	IRC_PARSE_COMMAND("352",	cmd, ParseNumberReply_Reply7Parameter);	// RPL_WHOREPLY
	IRC_PARSE_COMMAND("315",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ENDOFWHO
	IRC_PARSE_COMMAND("364",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_LINKS
	IRC_PARSE_COMMAND("365",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ENDOFLINKS
	IRC_PARSE_COMMAND("367",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_BANLIST
	IRC_PARSE_COMMAND("368",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ENDOFBANLIST
	IRC_PARSE_COMMAND("371",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_INFO
	IRC_PARSE_COMMAND("374",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_ENDOFINFO
	IRC_PARSE_COMMAND("381",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_YOUREOPER
	IRC_PARSE_COMMAND("382",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_REHASHING
	IRC_PARSE_COMMAND("391",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_TIME
	IRC_PARSE_COMMAND("392",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_USERSTART
	IRC_PARSE_COMMAND("393",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_USERS
	IRC_PARSE_COMMAND("394",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_ENDOFUSERS
	IRC_PARSE_COMMAND("395",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_NOUSERS	
	IRC_PARSE_COMMAND("200",	cmd, ParseNumberReply_Reply4Parameter);	// RPL_TRACELINK
	IRC_PARSE_COMMAND("201",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACECONNECTING
	IRC_PARSE_COMMAND("202",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACEHANDSHAKE
	IRC_PARSE_COMMAND("203",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACEUNKNOWN
	IRC_PARSE_COMMAND("204",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACEOPERATOR
	IRC_PARSE_COMMAND("205",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACEUSER
	IRC_PARSE_COMMAND("206",	cmd, ParseNumberReply_Reply6Parameter);	// RPL_TRACESERVER
	IRC_PARSE_COMMAND("208",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACENEWTYPE
	IRC_PARSE_COMMAND("261",	cmd, ParseNumberReply_Reply3Parameter);	// RPL_TRACELOG
	IRC_PARSE_COMMAND("211",	cmd, ParseNumberReply_Reply7Parameter);	// RPL_STATSLINKINFO
	IRC_PARSE_COMMAND("212",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_STATSCOMMANDS
	IRC_PARSE_COMMAND("213",	cmd, ParseNumberReply_Reply6Parameter);	// RPL_STATSCLINE
	IRC_PARSE_COMMAND("214",	cmd, ParseNumberReply_Reply6Parameter);	// RPL_STATSNLINE
	IRC_PARSE_COMMAND("215",	cmd, ParseNumberReply_Reply6Parameter);	// RPL_STATSILINE
	IRC_PARSE_COMMAND("216",	cmd, ParseNumberReply_Reply6Parameter);	// RPL_STATSKLINE
	IRC_PARSE_COMMAND("218",	cmd, ParseNumberReply_Reply5Parameter);	// RPL_STATSYLINE
	IRC_PARSE_COMMAND("219",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ENDOFSTATS
	IRC_PARSE_COMMAND("241",	cmd, ParseNumberReply_Reply5Parameter);	// RPL_STATSLLINE
	IRC_PARSE_COMMAND("242",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_STATSUPTIME
	IRC_PARSE_COMMAND("243",	cmd, ParseNumberReply_Reply4Parameter);	// RPL_STATSOLINE
	IRC_PARSE_COMMAND("244",	cmd, ParseNumberReply_Reply4Parameter);	// RPL_STATSHLINE
	IRC_PARSE_COMMAND("256",	cmd, ParseNumberReply_Reply2Parameter);	// RPL_ADMINME
	IRC_PARSE_COMMAND("257",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_ADMINLOC1
	IRC_PARSE_COMMAND("258",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_ADMINLOC2
	IRC_PARSE_COMMAND("259",	cmd, ParseNumberReply_Reply1Parameter);	// RPL_ADMINEMAIL

	IRC_PARSE_COMMAND("042",	cmd, ParseNumberReply_Reply1Parameter);	// None standard "unique-id" reply.
	
	IRC_PARSE_COMMAND("292",	cmd, ParseNumberReply_Reply1Parameter);	// None standard help reply used by UnrealIRCd
	IRC_PARSE_COMMAND("290",	cmd, ParseNumberReply_Reply1Parameter);	// None standard help reply used by UnrealIRCd

	ParseUnknown(cmd);
}

/// <summary>
///	  Command: PING
/// Parameters: <server1> [<server2>]
/// </summary>
void IRCServer::ParsePing(IRCCommand^ cmd)
{
	if (m_disconnecting == true)
	{
		ForceDisconnect();
		return;
	}

	SendCommand(IRCCommand::Create("PONG", cmd->Parameters->GetAt(0)));
}

/// <summary>
///    Command: NOTICE
/// Parameters: <nickname> <text>
/// </summary>
void IRCServer::ParseNotice(IRCCommand^ cmd)
{
	Platform::String^ reciever     = cmd->Parameters->GetAt(0);
	Platform::String^ message      = cmd->Parameters->GetAt(1);
	bool			  fromServer   = (cmd->PrefixUserName == "");
	bool			  targetIsUser = (reciever->Data()[0] != '#');


	// Sent as a user-to-user message.
	if (targetIsUser == true)
	{
		// Sent by server or user?
		if (fromServer == true)
		{
			AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, cmd->Parameters->GetAt(1), nullptr));
		}
		else
		{
			IRCUser^ user = GetUserByName(cmd->PrefixName);

			// Create new user to represent the sender?
			if (user == nullptr)
			{
				IRCUser^ user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
				AddUser(user);
			}

			// Find previous channel we are chatting to the user on.
			IRCChannel^ channel = GetChannelByName(cmd->PrefixName);
			bool newChannel = false;
			if (channel == nullptr)
			{			
				channel = ref new IRCChannel(this, cmd->PrefixName, IRCChannelType::IRC_CHANNEL_TYPE_USER);
				m_channels->Append(channel);
				newChannel = true;
			}

			// Send notice to a "user" window.
			AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, message, user));	

			// Did we just join this?
			if (newChannel == true)
			{
				// Get our user.
				IRCUser^ user = GetUserByName(m_currentNickname);
				if (user == nullptr)
				{
					IRCUser^ user = ref new IRCUser(this, m_currentNickname, "", m_currentNickname);
					AddUser(user);
				}

				OnJoinChannel(this, channel, user);
			}
		}
	}

	// Sending to a channel.
	else
	{
		IRCUser^ user = GetUserByName(cmd->PrefixName);

		// Create new user to represent the sender?
		if (user == nullptr)
		{
			IRCUser^ user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
			AddUser(user);
		}

		// Find channel we are sending to.
		IRCChannel^ channel = GetChannelByName(reciever);
		if (channel == nullptr)
		{			
			channel = ref new IRCChannel(this, reciever, IRCChannelType::IRC_CHANNEL_TYPE_USER);
			m_channels->Append(channel);
		}

		// Send notice to a channel window.
		AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, message, user));	
	}
}

/// <summary>
///    Command: PRIVMSG
/// Parameters: <receiver>{,<receiver>} <text to be sent>
/// </summary>
void IRCServer::ParsePrivMsg(IRCCommand^ cmd)
{
	Platform::String^ reciever     = cmd->Parameters->GetAt(0);
	Platform::String^ message      = cmd->Parameters->GetAt(1);
	bool			  fromServer   = (cmd->PrefixUserName == "");
	bool			  targetIsUser = (reciever->Data()[0] != '#');

	// Sent as a user-to-user message.
	if (targetIsUser == true)
	{
		// Sent by server or user?
		if (fromServer == true)
		{
			AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(1), nullptr));
		}
		else
		{
			// Create new user to represent the sender?
			IRCUser^ user = GetUserByName(cmd->PrefixName);
			if (user == nullptr)
			{
				user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
				AddUser(user);
			}
			
			// Parse CTCP.
			if (ParseCTCP(message, user) == true)
			{
				return;
			}

			// Find previous channel we are chatting to the user on.
			IRCChannel^ channel = GetChannelByName(cmd->PrefixName);
			bool newChannel = false;
			if (channel == nullptr)
			{			
				channel = ref new IRCChannel(this, cmd->PrefixName, IRCChannelType::IRC_CHANNEL_TYPE_USER);
				m_channels->Append(channel);			
				newChannel = true;
			}

			// Send notice to a "user" window.
			AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, message, user));	
			
			// Get our user.
			IRCUser^ ouruser = GetUserByName(m_currentNickname);
			if (ouruser == nullptr)
			{
				ouruser = ref new IRCUser(this, m_currentNickname, "", m_currentNickname);
				AddUser(ouruser);
			}

			// Did we just join this?
			if (newChannel == true)
			{
				user->AddActiveChannel(channel);
				ouruser->AddActiveChannel(channel);

				OnJoinChannel(this, channel, ouruser);
			}

			// Reply with away message?
			if (m_away == true)
			{
				Platform::String^ awayMessage = m_awayMessage;
				if (awayMessage == "")
				{
					awayMessage = "I am currently away from my computer.";
				}
				
				SendCommand(IRCCommand::Create("PRIVMSG", user->Name, awayMessage));
				AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, awayMessage, ouruser));	
			}
		}
	}

	// Sending to a channel.
	else
	{
		IRCUser^ user = GetUserByName(cmd->PrefixName);

		// Create new user to represent the sender?
		if (user == nullptr)
		{
			IRCUser^ user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
			AddUser(user);
		}

		// Find channel we are sending to.
		IRCChannel^ channel = GetChannelByName(reciever);
		if (channel == nullptr)
		{			
			channel = ref new IRCChannel(this, reciever, IRCChannelType::IRC_CHANNEL_TYPE_USER);
			m_channels->Append(channel);
		}

		// Send notice to channel.
		AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, message, user));	
	}
}

/// <summary>
/// Checks if a message is a CTCP 
/// </summary>
bool IRCServer::ParseCTCP(Platform::String^ message, IRCUser^ sender)
{
	// Is it in CTCP format? (\1action\1)
	if (message->Data()[0] != '\1')
	{
		return false;
	}

	// Get the rest of the reply.
	Platform::String^ command = "";
	for (unsigned int i = 1; i < message->Length(); i++)
	{
		wchar_t chr = message->Data()[i];
		if (chr == ' ' || chr == '\1')
		{
			break;
		}
		else
		{
			command += chr;
		}
	}

	// What is the command?
	if (StringHelper::ToUpperCase(command) == "ACTION")
	{
		return false; // This is dealt with in the actual message parsing, so ignore it.
	}
	else if (StringHelper::ToUpperCase(command) == "FINGER")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "FINGER :" + (m_userRealName == "" ? m_currentNickname : m_userRealName) + wchar_t(1));
		return true;
	}
	else if (StringHelper::ToUpperCase(command) == "VERSION")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "VERSION IRC-Messenger:1.0:Windows8" + wchar_t(1));
		return true;
	}
	else if (StringHelper::ToUpperCase(command) == "SOURCE")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "SOURCE twindrills.com:uploads:ircmessenger.zip" + wchar_t(1));
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "SOURCE" + wchar_t(1));
		return true;
	}
	else if (StringHelper::ToUpperCase(command) == "PING")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + message);		
		return true;
	}
	else if (StringHelper::ToUpperCase(command) == "USERINFO")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "USERINFO :None Provided" + wchar_t(1));
		return true;
	}
	else if (StringHelper::ToUpperCase(command) == "CLIENTINFO")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "CLIENTINFO :ACTION FINGER VERSION SOURCE PING USERINFO CLIENTINFO ERRMSG TIME" + wchar_t(1));
		return true;
	}
	else if (StringHelper::ToUpperCase(command) == "ERRMSG")
	{
		return false; // Ignore its a reply, just let it be output to the server window as normal.
	}
	else if (StringHelper::ToUpperCase(command) == "TIME")
	{
		SendMessageLine("NOTICE " + sender->Name + " :" + wchar_t(1) + "TIME :UNKNOWN" + wchar_t(1)); // TODO: Insert correct time.
		return true;
	}

	return false;
}

/// <summary>
///    Command: PART
/// Parameters: <channel>{,<channel>}
/// </summary>
void IRCServer::ParsePart(IRCCommand^ cmd)
{	
	// Grab the channel this join is refering to.
	IRCChannel^ channel = GetChannelByName(cmd->Parameters->GetAt(0));
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, cmd->Parameters->GetAt(0), IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}
	
	// Find/Create user.
	IRCUser^ user = GetUserByName(cmd->PrefixName);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
		AddUser(user);
	}
	
	// Remove user from channel.
	user->RemoveActiveChannel(channel);

	// x left #x.
	AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, cmd->PrefixName + " left " + channel->Name));

	// If its us thats leaving this channel, dispose of channel.
	if (cmd->PrefixName == m_currentNickname)
	{
		DisposeChannel(channel, false);
	}

	// Tell everyone a user left this channel.
	OnLeaveChannel(this, channel, user);
}

/// <summary>
///    Command: QUIT
/// Parameters: "message"
/// </summary>
void IRCServer::ParseQuit(IRCCommand^ cmd)
{	
	// Find/Create user.
	IRCUser^ user = GetUserByName(cmd->PrefixName);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
		AddUser(user);
	}
	
	// Remove user from channel.
	while (user->ActiveChannels->Size > 0)
	{
		LibIRC::IRCChannel^ channel = user->ActiveChannels->GetAt(0);
		
		Platform::String^ quitMessage = cmd->Parameters->Size <= 0 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 1); 

		// x left #x.
		AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, cmd->PrefixName + " quit" + (quitMessage == "" ? "" : " (" + quitMessage + ")")));

		user->RemoveActiveChannel(channel);

		// If its us thats leaving this channel, dispose of channel.
		if (cmd->PrefixName == m_currentNickname)
		{
			DisposeChannel(channel, false);
		}

		// Tell everyone a user left this channel.
		OnLeaveChannel(this, channel, user);
	}
}

/// <summary>
///    Command: JOIN
/// Parameters: <channel>{,<channel>} [<key>{,<key>}]
/// </summary>
void IRCServer::ParseJoin(IRCCommand^ cmd)
{		
	// Grab the channel this join is refering to.
	IRCChannel^ channel = GetChannelByName(cmd->Parameters->GetAt(0));
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, cmd->Parameters->GetAt(0), IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}
	
	// Find/Create user.
	IRCUser^ user = GetUserByName(cmd->PrefixName);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, cmd->PrefixUserName, cmd->PrefixHost, cmd->PrefixName);
		AddUser(user);
	}

	// Add user to this channel.
	user->AddActiveChannel(channel);

	// x joined #x.
	AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, cmd->PrefixName + " joined " + channel->Name));

	// Tell everyone a user joined this channel (if this is us, then we throw the join event after we recieve the user list).
	if (cmd->PrefixName != m_currentNickname)
	{
		OnJoinChannel(this, channel, user);
	}
}

/// <summary>
///    Command: NICK
/// Parameters: <nickname> [ <hopcount> ]
/// </summary>
void IRCServer::ParseNick(IRCCommand^ cmd)
{
	String^ oldNickname = cmd->PrefixName;
	String^ newNickname = cmd->Parameters->GetAt(0);
	
	// Find/Create user.
	IRCUser^ user = GetUserByName(oldNickname);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, oldNickname, cmd->PrefixHost, oldNickname);
		AddUser(user);
	}

	// Rename user.
	user->NickName = newNickname;
	
	m_usersMap->Remove(StringHelper::ToUpperCase(oldNickname));	
	m_usersMap->Insert(StringHelper::ToUpperCase(newNickname), user);
	
	// Change name of query channels.
	IRCChannel^ channel = GetChannelByName(oldNickname);
	if (channel != nullptr)
	{
		channel->Name		= newNickname;
	}

	// x is now known as #x.
	for (unsigned int i = 0; i < user->ActiveChannels->Size; i++)
	{
		AddMessage(ref new IRCMessage(user->ActiveChannels->GetAt(i), cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, oldNickname + " is now known as " + newNickname));
	}

	// Is it our username being changed?
	if (oldNickname == m_currentNickname)
	{
		m_currentNickname = newNickname;
		AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, "You are now known as " + newNickname));
	}

	OnUserNicknameChanged(this, user);
}

/// <summary>
///    Command: MODE
/// </summary>
void IRCServer::ParseMode(IRCCommand^ cmd)
{
	// User Mode
	// Parameters: <nickname> {[+|-]|i|w|s|o}

	if (cmd->Parameters->Size == 2)
	{
		Platform::String^ nickname = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
		Platform::String^ mode     = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);

		// Find/Create User.
		IRCUser^ user = GetUserByName(nickname);
		if (user == nullptr)
		{
			user = ref new IRCUser(this, nickname, "", nickname);
			AddUser(user);
		}

		// Broadcast a message.			
		AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, "Mode set to " + mode));

		user->SetMode(mode);
	}

	// Channel Mode
	// Parameters: <channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user>] [<ban mask>]
	else
	{	
		Platform::String^ channelName = cmd->Parameters->GetAt(0);
		Platform::String^ mode        = cmd->Parameters->GetAt(1);
		Platform::String^ extra       = cmd->Parameters->Size >= 3 ? cmd->Parameters->GetAt(2) : "";

		LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
		if (channel == nullptr)
		{			
			channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
			m_channels->Append(channel);
		}

		// If its a user mode, then lets take note of it.
		if (mode == "+o" || mode == "-o" ||
			mode == "+h" || mode == "-h" ||
			mode == "+v" || mode == "-v" ||
			mode == "+q" || mode == "-q" ||
			mode == "+a" || mode == "-a")
		{
			// Find/Create User.
			IRCUser^ user = GetUserByName(extra);
			if (user == nullptr)
			{
				user = ref new IRCUser(this, extra, "", extra);
				AddUser(user);
			}

			user->SetChannelMode(channel, mode);
			OnUserChannelModeChanged(this, channel, user);
		}

		// Broadcast a message.			
		AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, cmd->PrefixName + " set mode: " + mode + " " + extra));
	}
}

/// <summary>
///    Command: TOPIC
/// Parameters: <channel> [<topic>]
/// </summary>
void IRCServer::ParseTopic(IRCCommand^ cmd)
{
	Platform::String^ channelName = cmd->Parameters->GetAt(0);
	Platform::String^ topic       = cmd->Parameters->GetAt(1);

	LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}
	
	AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, cmd->PrefixName + " changed topic to: " + topic));
}

/// <summary>
///    Command: KICK
/// Parameters: <channel> <user> [<comment>]
/// </summary>
void IRCServer::ParseKick(IRCCommand^ cmd)
{
	Platform::String^ channelName = cmd->Parameters->GetAt(0);
	Platform::String^ nickname    = cmd->Parameters->GetAt(1);
	Platform::String^ reason      = cmd->Parameters->Size >= 3 ? cmd->Parameters->GetAt(2) : "";

	LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}

	// Find/Create User.
	IRCUser^ user = GetUserByName(nickname);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, nickname, "", nickname);
		AddUser(user);
	}

	AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, cmd->PrefixName + " kicked " + nickname + (reason != "" ? " (" + reason + ")" : "")));

	// Is this us? :(
	if (nickname == m_currentNickname)
	{		
		// Add an explanation in server window.
		AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, cmd->PrefixName + " kicked you from " + channelName + (reason != "" ? " (" + reason + ")" : "")));

		// Dispose of channel resources.
		DisposeChannel(channel, false);

		// Tell everyone a user left this channel.
		OnLeaveChannel(this, channel, user);
	}
}

/// <summary>
///    Command: 001
/// Parameters: "Welcome to the Internet Relay Network nickname"
/// </summary>
void IRCServer::ParseNumberReply_Welcome(IRCCommand^ cmd)
{			
	// Try and reconnect to all channels (this is for when we are reconnecting).
	for (unsigned int i = 0; i < m_channels->Size; i++)
	{
		IRCChannel^ channel = m_channels->GetAt(i);
		if (channel->Type == IRCChannelType::IRC_CHANNEL_TYPE_NORMAL)
		{
			SendMessageLine("JOIN "+channel->Name);
		}
	}

	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 002
/// Parameters: "Your host is server, running version ver"
/// </summary>
void IRCServer::ParseNumberReply_ServerVersion(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 003
/// Parameters: "This server was created datetime"
/// </summary>
void IRCServer::ParseNumberReply_ServerDate(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 004
/// Parameters: <server> <ver> <usermode> <chanmode>
/// </summary>
void IRCServer::ParseNumberReply_ServerInfo(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 4) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 3) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 2) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 351
/// Parameters: <version>.<debuglevel> <server> :<comments>
/// </summary>
void IRCServer::ParseNumberReply_VersionReply(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 3) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 2) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 005
/// Parameters: "commands"
/// </summary>
void IRCServer::ParseNumberReply_ServerCommands(IRCCommand^ cmd)
{
	Platform::String^ data = "";
	for (unsigned int i = 1; i < cmd->Parameters->Size; i++)
	{
		if (data != "")
		{
			data += " ";
		}
		data += cmd->Parameters->GetAt(i);
	}
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, data));
}

/// <summary>
///    Command: 251
/// Parameters: "There are user users and invis invisible on serv servers"
/// </summary>
void IRCServer::ParseNumberReply_UserCount(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 252
/// Parameters: <num> :operator(s) online
/// </summary>
void IRCServer::ParseNumberReply_OperatorCount(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 2) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 253
/// Parameters: <num> :unknown connection(s)
/// </summary>
void IRCServer::ParseNumberReply_UnknownConnections(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 2) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 254
/// Parameters: <num> :channels formed
/// </summary>
void IRCServer::ParseNumberReply_ChannelCount(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 2) + " " + cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 255
/// Parameters: "I have user clients and serv servers"
/// </summary>
void IRCServer::ParseNumberReply_ConnectionCount(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 265
/// Parameters: "Current local users: curr Max: max"
/// </summary>
void IRCServer::ParseNumberReply_LocalUserCount(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 266
/// Parameters: "Current global users: curr Max: max"
/// </summary>
void IRCServer::ParseNumberReply_GlobalUserCount(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}

/// <summary>
///    Command: 375
/// Parameters: "server Message of the Day -"
/// </summary>
void IRCServer::ParseNumberReply_MOTDStart(IRCCommand^ cmd)
{
	m_motd = cmd->Parameters->GetAt(cmd->Parameters->Size - 1) + "\n";
}

/// <summary>
///    Command: 372/377/378
/// Parameters: "message"
/// </summary>
void IRCServer::ParseNumberReply_MOTDLine(IRCCommand^ cmd)
{
	m_motd += cmd->Parameters->GetAt(cmd->Parameters->Size - 1) + "\n";
}

/// <summary>
///    Command: 376
/// Parameters: "message"
/// </summary>
void IRCServer::ParseNumberReply_MOTDEnd(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, m_motd));
}

/// <summary>
///    Command: 332
/// Parameters: "<channel> :<topic>"
/// </summary>
void IRCServer::ParseNumberReply_Topic(IRCCommand^ cmd)
{
	String^ channelName = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ topic       = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);

	LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}

	AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, "Topic is " + topic));
}

/// <summary>
///    Command: 333
/// Parameters: "<channel> <user> <time>"
/// </summary>
void IRCServer::ParseNumberReply_TopicTime(IRCCommand^ cmd)
{	
	String^ channelName = cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	String^ user        = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ time        = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);

	LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}

	// TODO: Convert from timestamp to friendly time.
	AddMessage(ref new IRCMessage(channel, cmd, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, "Topic set by " + user + " at timestamp " + time));
}

/// <summary>
///    Command: 353
/// Parameters: "<channel> :[[@|+]<nick> [[@|+]<nick> [...]]]"
/// </summary>
void IRCServer::ParseNumberReply_UserList(IRCCommand^ cmd)
{
	String^ channelName = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ userList    = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);

	LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}

	// Clear all users from this channel if this is the first user list reply.
	if (channel->ParsingUserList == false)
	{
		for (unsigned int i = 0; i < m_users->Size; i++)
		{
			IRCUser^ user = m_users->GetAt(i);
			unsigned int index = 0;

			user->RemoveActiveChannel(channel);
		}
	}

	// Add all users to this channel.
	String^ partialUsername = "";

	for (unsigned int i = 0; i < userList->Length() + 1; i++)
	{
		bool parsePartial = false;

		if (i == userList->Length())
		{
			parsePartial = true;
		}
		else
		{
			wchar_t chr = userList->Data()[i];
			if (chr == ' ')
			{
				parsePartial = true;
			}
			else
			{
				partialUsername += chr;
			}
		}

		if (parsePartial == true && partialUsername != "")
		{
			// Seperate the "mode" from the "nickname".
			String^ mode = "";
			String^ nick = "";

			for (unsigned int j = 0; j < partialUsername->Length(); j++)
			{
				wchar_t chr = partialUsername->Data()[j];

				if (nick != "")
				{
					nick += chr;
				}
				else
				{
					if (chr == '+' || chr == '@' || chr == '~' || chr == '&' || chr == '%')
					{
						mode += chr;
					}
					else
					{
						nick += chr;
					}
				}
			}

			// Find/Create User.
			IRCUser^ user = GetUserByName(nick);
			if (user == nullptr)
			{
				user = ref new IRCUser(this, nick, "", nick);
				AddUserNoSorting(user);
			}

			// Add to active channels.
			unsigned int index = 0;
			user->AddActiveChannel(channel);

			// Add mode to user.
			for (unsigned int j = 0; j < mode->Length(); j++)
			{
				wchar_t chr = mode->Data()[j];
				if (chr == '@')
				{
					user->SetChannelMode(channel, "+o");
				}
				else if (chr == '%')
				{
					user->SetChannelMode(channel, "+h");
				}
				else if (chr == '+')
				{
					user->SetChannelMode(channel, "+v");
				}
				else if (chr == '~')
				{
					user->SetChannelMode(channel, "+q");
				}
				else if (chr == '&')
				{
					user->SetChannelMode(channel, "+a");
				}
			}
			
			partialUsername = "";
		}
	}

	channel->ParsingUserList = true;
}

/// <summary>
///    Command: 366
/// Parameters: "<channel> :End of /NAMES list"
/// </summary>
void IRCServer::ParseNumberReply_EndOfUserList(IRCCommand^ cmd)
{
	String^ channelName = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);

	LibIRC::IRCChannel^ channel = GetChannelByName(channelName);
	if (channel == nullptr)
	{			
		channel = ref new IRCChannel(this, channelName, IRCChannelType::IRC_CHANNEL_TYPE_NORMAL);
		m_channels->Append(channel);
	}

	channel->ParsingUserList = false;

	// Sort the username list we have just recieved.
	SortUsers();

	// Throw up an event that we have joined the channel.
	IRCUser^ user = GetUserByName(m_currentNickname);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, m_currentNickname, "", m_currentNickname);
		AddUser(user);
	}
	OnJoinChannel(this, channel, user);
}

/// <summary>
///    Command: 433
/// Parameters: "<nick> :Nickname is already in use"
/// </summary>
void IRCServer::ParseNumberReply_NickNameInUse(IRCCommand^ cmd)
{
	// Give the user a random nickname until they choose a new one.
	m_currentNickname = "UnnamedUser" + (rand() % 10000);
	SendMessageLine("NICK " + m_currentNickname);
}

/// <summary>
///    Command: 221
/// Parameters:  "<user mode string>"
/// </summary>
void IRCServer::ParseNumberReply_UserModeIs(IRCCommand^ cmd)
{
	Platform::String^ nickname = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	Platform::String^ mode     = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);

	// Find/Create User.
	IRCUser^ user = GetUserByName(nickname);
	if (user == nullptr)
	{
		user = ref new IRCUser(this, nickname, "", nickname);
		AddUser(user);
	}

	user->SetMode(mode);
}

/// <summary>
///    Command: ###
/// Parameters: 
/// </summary>
void IRCServer::ParseNumberReply_ErrorReply0Parameter(IRCCommand^ cmd)
{
	String^ msg = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg, nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> 
/// </summary>
void IRCServer::ParseNumberReply_ErrorReply1Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> <param2>
/// </summary>
void IRCServer::ParseNumberReply_ErrorReply2Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ param2	= cmd->Parameters->Size < 3 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+" "+param2+")", nullptr));
}

/// <summary>
///    Command: 301
/// Parameters: <nick> <message>
/// </summary>
void IRCServer::ParseNumberReply_AwayMessage(IRCCommand^ cmd)
{
	String^ nick = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ msg = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, nick+": "+msg, nullptr));
}

/// <summary>
///    Command: 305
/// Parameters: <message>
/// </summary>
void IRCServer::ParseNumberReply_NotAway(IRCCommand^ cmd)
{
	String^ msg = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_ACTIVE, msg, nullptr));
}

/// <summary>
///    Command: 306
/// Parameters: <message>
/// </summary>
void IRCServer::ParseNumberReply_NowAway(IRCCommand^ cmd)
{
	String^ msg = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_PASSIVE, msg, nullptr));
}

/// <summary>
///    Command: 331
/// Parameters: Channel :Users Name
/// </summary>
void IRCServer::ParseNumberReply_ListStart(IRCCommand^ cmd)
{
	m_list = "";
	m_listLength = 0;
}

/// <summary>
///    Command: 322
/// Parameters: <channel> <# visible>: topic
/// </summary>
void IRCServer::ParseNumberReply_List(IRCCommand^ cmd)
{
	String^ channel = cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	String^ visible = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ topic   = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);

	// No topic specified.
	if (cmd->Parameters->Size <= 3)
	{
		channel = cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
		visible = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
		topic   = "";
	}

	unsigned int segment_limit = 20;

	m_list += StringHelper::RightPad(channel, ' ', 25) + StringHelper::RightPad(visible + " users", ' ', 10) + StringHelper::Limit(topic, 70) + (m_listLength + 1 >= segment_limit ? "" : "\n");
	m_listLength += 1;

	if (m_listLength >= segment_limit)
	{
		AddMessage(ref new IRCMessage(nullptr, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, m_list));
		m_listLength = 0;
		m_list = "";
	}
}

/// <summary>
///    Command: 323
/// Parameters: :End of /LIST
/// </summary>
void IRCServer::ParseNumberReply_ListEnd(IRCCommand^ cmd)
{
	if (m_listLength > 0)
	{
		AddMessage(ref new IRCMessage(nullptr, cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, m_list));
	}
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply1Parameter(IRCCommand^ cmd)
{
	String^ msg = cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg, nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply2Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply3Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ param2	= cmd->Parameters->Size < 3 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+" "+param2+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply4Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ param2	= cmd->Parameters->Size < 3 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	String^ param3	= cmd->Parameters->Size < 4 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 4);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+" "+param2+" "+param3+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply5Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ param2	= cmd->Parameters->Size < 3 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	String^ param3	= cmd->Parameters->Size < 4 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 4);
	String^ param4	= cmd->Parameters->Size < 5 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 5);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+" "+param2+" "+param3+" "+param4+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply6Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ param2	= cmd->Parameters->Size < 3 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	String^ param3	= cmd->Parameters->Size < 4 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 4);
	String^ param4	= cmd->Parameters->Size < 5 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 5);
	String^ param5	= cmd->Parameters->Size < 6 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 6);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+" "+param2+" "+param3+" "+param4+" "+param5+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <param1> ...
/// </summary>
void IRCServer::ParseNumberReply_Reply7Parameter(IRCCommand^ cmd)
{
	String^ msg		= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ param1	= cmd->Parameters->Size < 2 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	String^ param2	= cmd->Parameters->Size < 3 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 3);
	String^ param3	= cmd->Parameters->Size < 4 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 4);
	String^ param4	= cmd->Parameters->Size < 5 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 5);
	String^ param5	= cmd->Parameters->Size < 6 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 6);
	String^ param6	= cmd->Parameters->Size < 7 ? "" : cmd->Parameters->GetAt(cmd->Parameters->Size - 7);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " ("+param1+" "+param2+" "+param3+" "+param4+" "+param5+" "+param6+")", nullptr));
}

/// <summary>
///    Command: ###
/// Parameters: <channel> <message>
/// </summary>
void IRCServer::ParseNumberReply_ErrorCantJoinChannel(IRCCommand^ cmd)
{
	String^ channelname	= cmd->Parameters->GetAt(cmd->Parameters->Size - 1);
	String^ msg			= cmd->Parameters->GetAt(cmd->Parameters->Size - 2);
	AddMessage(ref new IRCMessage(nullptr, nullptr, IRCMessageType::IRC_MESSAGE_TYPE_NOTICE, msg + " (" + channelname + ")", nullptr));

	// Grab the channel this join is refering to.
	IRCChannel^ channel = GetChannelByName(channelname);
	if (channel != nullptr)
	{					
		IRCUser^ user = GetUserByName(m_currentNickname);
		if (user == nullptr)
		{
			user = ref new IRCUser(this, m_currentNickname, "", m_currentNickname);
			AddUser(user);
		}	

		DisposeChannel(channel, false);
		OnLeaveChannel(this, channel, user);
	}
}

/// <summary>
///    Command: ERROR
/// Parameters: <error message>
/// </summary>
void IRCServer::ParseError(IRCCommand^ cmd)
{
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(0)));
	if (m_disconnecting == true)
	{
		ForceDisconnect();
	}
	else
	{
		CloseStream(true);
	}
}

/// <summary>
/// Parses any commands recieved that are unknown.
/// </summary>
void IRCServer::ParseUnknown(IRCCommand^ cmd)
{
	// No idea what this command is, might as well dump it in the server channel so we can debug it.
	AddMessage(ref new IRCMessage(GetChannelByName("Server"), cmd, IRCMessageType::IRC_MESSAGE_TYPE_NORMAL, cmd->Parameters->GetAt(cmd->Parameters->Size - 1)));
}
