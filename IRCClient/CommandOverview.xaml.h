//
// CommandOverview.xaml.h
// Declaration of the CommandOverview class
//

#pragma once

#include "Common\LayoutAwarePage.h" // Required by generated header
#include "CommandOverview.g.h"

namespace IRCClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	public ref class CommandOverview sealed
	{
	public:
		CommandOverview();

	protected:
		virtual void LoadState(Platform::Object^ navigationParameter,
			Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
		virtual void SaveState(Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ pageState) override;
	};
}
