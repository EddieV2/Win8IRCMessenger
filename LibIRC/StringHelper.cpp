// -----------------------------------------------------------------------------
// IRC Client (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#include "pch.h"
#include "StringHelper.h"

#include <cctype>

using namespace LibIRC;
using namespace Platform;

/// <summary>
/// Converts a string to upper case.
/// </summary>
Platform::String^ StringHelper::ToUpperCase(Platform::String^ cmd)
{
	Platform::String^ result = "";

	for (unsigned int i = 0; i < cmd->Length(); i++)
	{
		wchar_t chr = cmd->Data()[i];
		result += (wchar_t)std::toupper(chr);
	}

	return result;
}

/// <summary>
/// Converts a string to lower case.
/// </summary>
Platform::String^ StringHelper::ToLowerCase(Platform::String^ cmd)
{
	Platform::String^ result = "";

	for (unsigned int i = 0; i < cmd->Length(); i++)
	{
		wchar_t chr = cmd->Data()[i];
		result += (wchar_t)std::tolower(chr);
	}

	return result;
}

/// <summary>
/// Pads the right side of a string out with characters until it meets the desired length.
/// </summary>
Platform::String^ StringHelper::RightPad(Platform::String^ cmd, wchar_t padChar, unsigned int len)
{
	while (cmd->Length() < len)
	{
		cmd += padChar;
	}
	return cmd;
}

/// <summary>
/// Truncates the string if it goes over the given length.
/// </summary>
Platform::String^ StringHelper::Limit(Platform::String^ cmd, unsigned int len)
{
	if (cmd->Length() > len)
	{
		cmd = ref new Platform::String(cmd->Data(), len);
	}
	return cmd;
}