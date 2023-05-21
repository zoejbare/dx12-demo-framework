//
// Copyright (c) 2023, Zoe J. Bare
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
#pragma once

//---------------------------------------------------------------------------------------------------------------------

#include "../BuildSetup.h"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework
{
	class IAppView;

}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::IAppView
{
public:

	typedef IAppView* (*OnCreateFn)();

	IAppView() = default;
	IAppView(const IAppView&) = delete;
	IAppView(IAppView&&) = delete;
	virtual ~IAppView() {}

	IAppView& operator =(const IAppView&) = delete;
	IAppView& operator =(IAppView&&) = delete;

	/*!
	 * Initialize the app-view.
	 *
	 * @returns  True if app-view initialization was successful.
	 */
	virtual bool Initialize() = 0;

	/*!
	 * Run an iteration of the application main loop.
	 *
	 * @returns  True while the application should continue running; false when it's time for the application to exit.
	 */
	virtual bool MainLoopUpdate() = 0;

	//! Dispose of all app-view resources immediately prior to exiting the application.
	virtual void Shutdown() = 0;
};

//---------------------------------------------------------------------------------------------------------------------
