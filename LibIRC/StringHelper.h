// -----------------------------------------------------------------------------
// IRC Library (Systems Programming Project)
// Written by Tim Leonard
// -----------------------------------------------------------------------------
#pragma once

#include <pch.h>

namespace LibIRC
{

	/// <summary>
	/// Static class that stores some handy string helper functions.
	/// <summary>
	public ref class StringHelper sealed
	{
	public:

		static Platform::String^ ToUpperCase	(Platform::String^ cmd);
		static Platform::String^ ToLowerCase	(Platform::String^ cmd);
		static Platform::String^ RightPad		(Platform::String^ cmd, wchar_t padChar, unsigned int len);
		static Platform::String^ Limit			(Platform::String^ cmd, unsigned int len);
		
	};

}