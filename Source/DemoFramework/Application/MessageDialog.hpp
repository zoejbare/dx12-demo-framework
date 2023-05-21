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

#include "DialogAlert.hpp"
#include "DialogButton.hpp"
#include "DialogResult.hpp"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework
{
	class MessageDialog;
}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::MessageDialog
{
public:

	MessageDialog() = delete;
	MessageDialog(const MessageDialog&) = delete;
	MessageDialog(MessageDialog&&) = delete;

	/*!
	 * Display a native message box, blocking until the user has closed it.
	 *
	 * @param[in]  hWnd        Handle to the message box's parent window (null if no parent).
	 * @param[in]  alertType   Type of alert icon to show on the message box.
	 * @param[in]  buttonType  Type of button configuration to display on the message box.
	 * @param[in]  title       Text to show along the title bar of the message box.
	 * @param[in]  message     Text to show in the body of the message box.
	 */
	static DialogResult Show(
		HWND hWnd,
		DialogAlert alertType,
		DialogButton buttonType,
		const char* title,
		const char* message
	);
};

//---------------------------------------------------------------------------------------------------------------------
