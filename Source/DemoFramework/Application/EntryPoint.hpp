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

#include "AppView.hpp"

#include <locale.h>

//---------------------------------------------------------------------------------------------------------------------

#define DF_EXIT_CODE_SUCCESS               0
#define DF_EXIT_CODE_FATAL_ERROR          -1
#define DF_EXIT_CODE_INIT_FAILED           1
#define DF_EXIT_CODE_CREATE_CALLBACK_NULL  2
#define DF_EXIT_CODE_APP_VIEW_NULL         3

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework
{
	//! Initialize the application and run the main loop, returning an exit code when the application shuts down.
	DF_API int32_t RunApplication(IAppView::OnCreateFn onCreateAppView);

	//! Flag a fatal error in the program execution which will cause the application to exit with a fatal error code.
	DF_API void RaiseFatalError();

	//! Query the application backend to see if a fatal error was raised at any point.
	DF_API bool WasFatalErrorRaised();
}

//---------------------------------------------------------------------------------------------------------------------

