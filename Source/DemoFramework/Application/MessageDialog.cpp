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

#include "MessageDialog.hpp"

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::DialogResult DemoFramework::MessageDialog::Show(
	HWND hWnd,
	const DialogAlert alertType,
	const DialogButton buttonType,
	const char* const title,
	const char* const message
)
{
	uint32_t messageBoxFlags = 0;

	switch(alertType)
	{
		case DialogAlert::Info:    messageBoxFlags |= MB_ICONINFORMATION; break;
		case DialogAlert::Warning: messageBoxFlags |= MB_ICONWARNING;     break;
		case DialogAlert::Error:   messageBoxFlags |= MB_ICONERROR;       break;

		default:
			// This should never happen.
			assert(false);
			break;
	}

	switch(buttonType)
	{
		case DialogButton::Ok:                messageBoxFlags |= MB_OK;                break;
		case DialogButton::OkCancel:          messageBoxFlags |= MB_OKCANCEL;          break;
		case DialogButton::YesNo:             messageBoxFlags |= MB_YESNO;             break;
		case DialogButton::YesNoCancel:       messageBoxFlags |= MB_YESNOCANCEL;       break;
		case DialogButton::RetryCancel:       messageBoxFlags |= MB_RETRYCANCEL;       break;
		case DialogButton::AbortRetryIgnore:  messageBoxFlags |= MB_ABORTRETRYIGNORE;  break;
		case DialogButton::CancelTryContinue: messageBoxFlags |= MB_CANCELTRYCONTINUE; break;

		default:
			// This should never happen.
			assert(false);
			break;
	}

	// Show the message box.
	const int messageBoxResult = MessageBoxA(hWnd, message, title, messageBoxFlags);
	switch(messageBoxResult)
	{
		case IDOK:       return DialogResult::Ok;       break;
		case IDCANCEL:   return DialogResult::Cancel;   break;
		case IDYES:      return DialogResult::Yes;      break;
		case IDNO:       return DialogResult::No;       break;
		case IDRETRY:    return DialogResult::Retry;    break;
		case IDABORT:    return DialogResult::Abort;    break;
		case IDIGNORE:   return DialogResult::Ignore;   break;
		case IDTRYAGAIN: return DialogResult::TryAgain; break;
		case IDCONTINUE: return DialogResult::Continue; break;

		default:
			break;
	}

	return DialogResult::Unknown;
}

//---------------------------------------------------------------------------------------------------------------------
