// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include <pch.h>
#include "IRCChannel.h"
#include "IRCUser.h"
#include "StringHelper.h"

namespace LibIRC
{
	// Forward references.
	ref class IRCController;
	ref class IRCServer;
	ref class IRCCommand;
	ref class IRCChannel;
	ref class IRCMessage;
	ref class IRCUser;
	
	// Connection delegates.
	public delegate void IRCConnectSuccessDelegate			(IRCServer^ server);
	public delegate void IRCConnectFailureDelegate			(IRCServer^ server);
	public delegate void IRCDisconnectDelegate				(IRCServer^ server);
	public delegate void IRCMessageRecievedDelegate			(IRCServer^ server, IRCMessage^ message);
	public delegate void IRCJoinChannelDelegate				(IRCServer^ server, IRCChannel^ channel, IRCUser^ user);
	public delegate void IRCLeaveChannelDelegate			(IRCServer^ server, IRCChannel^ channel, IRCUser^ user);
	public delegate void IRCUserChannelModeChangedDelegate	(IRCServer^ server, IRCChannel^ channel, IRCUser^ user);
	public delegate void IRCUserNicknameChangedDelegate		(IRCServer^ server, IRCUser^ user);

	// General settings.
	#define IRC_SERVER_CONNECT_RETRY_INTERVAL_START	100
	#define IRC_SERVER_CONNECT_RETRY_INTERVAL_STEP	2
	#define IRC_SERVER_CONNECT_RETRY_INTERVAL_END	600000
	
	// Handy macro to shorten code invokation when parsing..
	#define IRC_PARSE_COMMAND(name, cmd, func)					if (cmd->Command == name) { func(cmd); return; }
	#define IRC_SEND_COMMAND (name, cmd, params, dest, func)	if (StringHelper::ToUpperCase(cmd) == StringHelper::ToUpperCase(name)) { func(dest, params); return; }

	// Declaration of command handler method prototype.		
	private delegate void CommandHandlerDelegate(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
	typedef void (IRCServer::*CommandHandlerFunctionPtr) (IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);

	/// <summary>
	/// This class is just used internally to make things slightly easier in the command dispatching.
	/// </summary>
	private struct IRCCommandHandler sealed
	{
	public:
		Platform::String^			Name;
		Platform::String^			Syntax;
		unsigned int				ParameterMin;
		unsigned int				ParameterMax;
		CommandHandlerFunctionPtr	Function;

		IRCCommandHandler(Platform::String^ name, unsigned int paramMin, unsigned int paramMax, CommandHandlerFunctionPtr func, Platform::String^ syntax)
		{
			Name				= name;
			ParameterMin		= paramMin;
			ParameterMax		= paramMax;
			Function			= func;
			Syntax				= syntax;
		}

	};

	/// <summary>
	/// The IRC server class manages code access to a given remote IRC server.
	///	Allowing the user to connect, disconnect and interact with the remote IRC server.
	/// </summary>
	public ref class IRCServer sealed
	{
	private:
		
		// ------------------------------------------------------------------------------------
		// Member variables.
		// ------------------------------------------------------------------------------------
		IRCController^								m_controller;

		bool										m_active;

		Platform::String^							m_name;
		Platform::String^							m_hostname;
		Platform::String^							m_port;

		Windows::Networking::HostName^				m_network_hostname;
		Windows::Networking::Sockets::StreamSocket^	m_socket;

		bool										m_connected;
		bool										m_connecting;
		bool										m_disconnecting;

		Windows::Storage::Streams::DataReader^		m_input_reader;
		Windows::Storage::Streams::DataWriter^		m_output_writer;
		Platform::String^							m_input_buffer;

		Platform::String^							m_username;
		Platform::String^							m_password;
		Platform::String^							m_userHostname;
		Platform::String^							m_userRealName;
		
		Platform::String^							m_currentNickname;

		Platform::String^							m_motd;
		Platform::String^							m_list;
		unsigned int								m_listLength;

		Platform::Collections::Vector<IRCChannel^>^					m_channels;
		Platform::Collections::Vector<IRCUser^>^					m_users;
		Platform::Collections::Map<Platform::String^, IRCUser^>^	m_usersMap;

		bool										m_away;
		Platform::String^							m_awayMessage;

		// ------------------------------------------------------------------------------------
		// Private methods.
		// ------------------------------------------------------------------------------------
		void CloseStream							(bool byError);
		
		void BeginSendLoop							();
		void SendLoop								();

		void BeginReadLoop							();
		void ReadLoop								();

		void ProcessInputData						(Platform::String^ data);
		void ParseInputLine							(Platform::String^ data);

		bool			  EatColorCodeChar			(Platform::String^ data, wchar_t min, wchar_t max, unsigned int& pos);
		Platform::String^ StripColorCodes			(Platform::String^ data);

		void SendAction								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendNick								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendMsg								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendOper								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendQuit								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendHelp								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendMOTD								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendList								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendChanServ							(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendNickServ							(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendMemoServ							(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendAway								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendBack								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendCTCP								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendInvite								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendKnock								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendMode								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendJoin								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendPart								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendPartAll							(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendTopic								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendRaw								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendBan								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendUnBan								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendKick								(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);
		void SendVersion							(IRCChannel^ destination, Platform::Collections::Vector<Platform::String^>^ parameters);

		bool ParseCTCP								(Platform::String^ message, IRCUser^ sender);

		void ParseCommand							(IRCCommand^ cmd);
		void ParsePing								(IRCCommand^ cmd);
		void ParseNotice							(IRCCommand^ cmd);
		void ParsePrivMsg							(IRCCommand^ cmd);
		void ParseError								(IRCCommand^ cmd);
		void ParseQuit								(IRCCommand^ cmd);
		void ParsePart								(IRCCommand^ cmd);
		void ParseJoin								(IRCCommand^ cmd);
		void ParseNick								(IRCCommand^ cmd);
		void ParseMode								(IRCCommand^ cmd);
		void ParseTopic								(IRCCommand^ cmd);
		void ParseKick								(IRCCommand^ cmd);
		void ParseNumberReply_Welcome				(IRCCommand^ cmd);
		void ParseNumberReply_ServerVersion			(IRCCommand^ cmd);
		void ParseNumberReply_ServerDate			(IRCCommand^ cmd);
		void ParseNumberReply_ServerInfo			(IRCCommand^ cmd);
		void ParseNumberReply_VersionReply			(IRCCommand^ cmd);
		void ParseNumberReply_ServerCommands		(IRCCommand^ cmd);
		void ParseNumberReply_UserCount				(IRCCommand^ cmd);
		void ParseNumberReply_OperatorCount			(IRCCommand^ cmd);
		void ParseNumberReply_UnknownConnections	(IRCCommand^ cmd);
		void ParseNumberReply_ChannelCount			(IRCCommand^ cmd);
		void ParseNumberReply_ConnectionCount		(IRCCommand^ cmd);
		void ParseNumberReply_LocalUserCount		(IRCCommand^ cmd);
		void ParseNumberReply_GlobalUserCount		(IRCCommand^ cmd);
		void ParseNumberReply_MOTDStart				(IRCCommand^ cmd);
		void ParseNumberReply_MOTDLine				(IRCCommand^ cmd);
		void ParseNumberReply_MOTDEnd				(IRCCommand^ cmd);
		void ParseNumberReply_Topic					(IRCCommand^ cmd);
		void ParseNumberReply_TopicTime				(IRCCommand^ cmd);
		void ParseNumberReply_UserList				(IRCCommand^ cmd);
		void ParseNumberReply_EndOfUserList			(IRCCommand^ cmd);
		void ParseNumberReply_NickNameInUse			(IRCCommand^ cmd);
		void ParseNumberReply_UserModeIs			(IRCCommand^ cmd);
		void ParseNumberReply_ErrorReply0Parameter	(IRCCommand^ cmd);
		void ParseNumberReply_ErrorReply1Parameter	(IRCCommand^ cmd);
		void ParseNumberReply_ErrorReply2Parameter	(IRCCommand^ cmd);

		void ParseNumberReply_AwayMessage			(IRCCommand^ cmd);
		void ParseNumberReply_NotAway				(IRCCommand^ cmd);
		void ParseNumberReply_NowAway				(IRCCommand^ cmd);
		void ParseNumberReply_ListStart				(IRCCommand^ cmd);
		void ParseNumberReply_List					(IRCCommand^ cmd);
		void ParseNumberReply_ListEnd				(IRCCommand^ cmd);
		void ParseNumberReply_Reply1Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_Reply2Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_Reply3Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_Reply4Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_Reply5Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_Reply6Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_Reply7Parameter		(IRCCommand^ cmd);
		void ParseNumberReply_ErrorCantJoinChannel	(IRCCommand^ cmd);

		void ParseUnknown							(IRCCommand^ cmd);
		
		void AddMessage								(IRCMessage^ msg);
		void AddUser								(IRCUser^ user);
		void AddUserNoSorting						(IRCUser^ user);
		void SortUsers								();

		void DisposeChannel							(IRCChannel^ channel, bool invokeLeave);
		void RecreateSocket							();

	public:
		
		// ------------------------------------------------------------------------------------
		// Public properties.
		// ------------------------------------------------------------------------------------
		
		/// <summary>
		/// Returns if this server is currently "active" (connecting/reconnecting/connected/etc)
		/// </summary>
		property bool Active
        {
            bool get()										{ return m_active; }
        }
		
		/// <summary>
		/// Returns true if we are connected to the server.
		/// </summary>
		property bool Connected
        {
            bool get()										{ return m_connected; }
        }
		
		/// <summary>
		/// Returns true if we are connecting to a server.
		/// </summary>
		property bool Connecting
        {
            bool get()										{ return m_connecting; }
        }
		
		/// <summary>
		/// Returns true if we are disconnecting from the server.
		/// </summary>
		property bool Disconnecting
        {
            bool get()										{ return m_disconnecting; }
        }

		/// <summary>
		/// Gets or sets the hostname of the server. Can only be done when not connected,
		/// if you attempt to do it whilst connected an exception will be thrown.
		/// </summary>
		property Platform::String^ Hostname
        {
            Platform::String^ get()							{ return m_hostname; }
            void			  set(Platform::String^ value)	
			{ 
				//if (m_connected == true)
				//{
				//	throw ref new Platform::Exception(0, "Attempt to perform action that requires IRC connection to be disconnected first.");
				//}
				m_hostname = value; 		
				try
				{
					m_network_hostname	= ref new Windows::Networking::HostName(m_hostname);
				}
				catch (Platform::Exception^ ex)
				{
					m_network_hostname	= ref new Windows::Networking::HostName("irc.example.com");
				}
			}
        }

		/// <summary>
		/// Gets or sets the port of the server. Can only be done when not connected,
		/// if you attempt to do it whilst connected an exception will be thrown.
		/// </summary>
		property Platform::String^ Port
        {
            Platform::String^ get()							{ return m_port; }
            void			  set(Platform::String^ value)	
			{ 
				//if (m_connected == true)
				//{
				//	throw ref new Platform::Exception(0, "Attempt to perform action that requires IRC connection to be disconnected first.");
				//}
				m_port = value; 
			}
        }

		/// <summary>
		/// Gets or sets the friendly-name of the server.
		/// </summary>
		property Platform::String^ Name
        {
            Platform::String^ get()							{ return m_name; }
			void			  set(Platform::String^ value)	{ m_name = value; }
		}
		
		/// <summary>
		/// Gets the current nickname of the user.
		/// </summary>
		property Platform::String^ CurrentNickName
        {
			Platform::String^ get()							{ return m_currentNickname; }
		}

		/// <summary>
		/// Gets or sets the username used when logging into the server. Can only be done when not connected,
		/// if you attempt to do it whilst connected an exception will be thrown.
		///	If you want to change the users nickname when connected, user the AsyncChangeNickname method.
		/// </summary>
		property Platform::String^ Username
        {
            Platform::String^ get()							{ return m_username; }
            void			  set(Platform::String^ value)	
			{ 
				//if (m_connected == true)
				//{
				//	throw ref new Platform::Exception(0, "Attempt to perform action that requires IRC connection to be disconnected first.");
				//}
				//else
				//{
					m_username = value; 
				//}
			}
        }

		/// <summary>
		/// Gets or sets the password used when logging into the server. Can only be done when not connected,
		/// if you attempt to do it whilst connected an exception will be thrown.
		///	If you want to identify with the password when connected, use the AsyncIndentifyUsingPassword method.
		/// </summary>
		property Platform::String^ Password
        {
            Platform::String^ get()							{ return m_password; }
            void			  set(Platform::String^ value)	
			{ 
				//if (m_connected == true)
				//{
				//	throw ref new Platform::Exception(0, "Attempt to perform action that requires IRC connection to be disconnected first.");
				//}
				//else
				//{
					m_password = value; 
				//}
			}
		}
		
		/// <summary>
		/// Gets or sets the users hostname, used when logging into the server. This does very little of intrest,
		/// and is ignored by the majority of servers, but can be used for legacy servers. 
		/// Can only be done when not connected, if you attempt to do it whilst connected an exception will be thrown.
		/// </summary>
		property Platform::String^ UserHostname
        {
            Platform::String^ get()							{ return m_userHostname; }
            void			  set(Platform::String^ value)	
			{ 
				//if (m_connected == true)
				//{
				//	throw ref new Platform::Exception(0, "Attempt to perform action that requires IRC connection to be disconnected first.");
				//}
				//else
				//{
					m_userHostname = value; 
				//}
			}
        }

		/// <summary>
		/// Gets or sets the users real name, used when logging into the server.
		/// Can only be done when not connected, if you attempt to do it whilst connected an exception will be thrown.
		/// </summary>
		property Platform::String^ UserRealName
        {           
			Platform::String^ get()							{ return m_userRealName; }           
			void			  set(Platform::String^ value)	
			{ 
				//if (m_connected == true)
				//{
				//	throw ref new Platform::Exception(0, "Attempt to perform action that requires IRC connection to be disconnected first.");
				//}
				//else
				//{
					m_userRealName = value; 
				//}
			}
        }
		
		/// <summary>
		/// Gets the list of servers held by this controller.
		/// </summary>
		property Windows::Foundation::Collections::IVector<IRCChannel^>^ Channels
        {
			Windows::Foundation::Collections::IVector<IRCChannel^>^ get() { return m_channels; }
        }

		/// <summary>
		/// Gets the list of users held by this controller.
		/// </summary>
		property Windows::Foundation::Collections::IVector<IRCUser^>^ Users
        {
			Windows::Foundation::Collections::IVector<IRCUser^>^ get() { return m_users; }
        }

		// ------------------------------------------------------------------------------------
		// Public delegates.
		// ------------------------------------------------------------------------------------
		event IRCConnectSuccessDelegate^			OnConnectSuccess;
		event IRCConnectFailureDelegate^			OnConnectFailure;
		event IRCDisconnectDelegate^				OnDisconnect;
		event IRCMessageRecievedDelegate^			OnMessageRecieved;
		event IRCJoinChannelDelegate^				OnJoinChannel;
		event IRCLeaveChannelDelegate^				OnLeaveChannel;
		event IRCUserChannelModeChangedDelegate^	OnUserChannelModeChanged;
		event IRCUserNicknameChangedDelegate^		OnUserNicknameChanged;

		// ------------------------------------------------------------------------------------
		// Public methods.
		// ------------------------------------------------------------------------------------
		IRCServer									(IRCController^ controller, Platform::String^ hostname, Platform::String^ port);
		virtual		~IRCServer						();

		void		AsyncConnect					();
		void		AsyncConnect					(bool keepRetrying, int retryInterval);
		void		AsyncDisconnect					();
		void		ForceDisconnect					();

		void		AsyncJoinChannel				(Platform::String^ name, Platform::String^ password);

		IRCChannel^ GetChannelByName				(Platform::String^ name);
		IRCUser^	GetUserByName					(Platform::String^ name);
		
		void		SendCommand						(IRCCommand^ data);
		void		SendMessageLine					(Platform::String^ data);

		void		ParseAndSendMessage				(IRCChannel^ destination, Platform::String^ message);
		void		SendMessageToChannel			(IRCChannel^ destination, Platform::String^ message);

		IRCChannel^	QueryUser						(IRCUser^ user);

		// Allow irc channel to access some of our non-public methods.
		friend ref class IRCChannel;
	};

}