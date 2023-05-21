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

#include "EntryPoint.hpp"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace prv {
	static bool fatalError = false;
}}

//---------------------------------------------------------------------------------------------------------------------

int32_t DemoFramework::RunApplication(IAppView::OnCreateFn onCreateAppView)
{
	// Initialize COM support for the calling thread.
	const HRESULT coInitResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	assert(coInitResult == S_OK); (void) coInitResult;

	// Switch the application locale to the system default.
	setlocale(LC_ALL, "");

	int32_t exitCode = DF_EXIT_CODE_SUCCESS;

	if(onCreateAppView)
	{
		// Initialize the app view.
		IAppView* const pAppView = onCreateAppView();
		if(pAppView)
		{
			// Initialize the application.
			if(pAppView->Initialize() && !prv::fatalError)
			{
				// Iterate over the program main loop until the application
				// says it's ready to exit or a fatal error occurs.
				while(!prv::fatalError && pAppView->MainLoopUpdate()) {}
			}
			else
			{
				exitCode = DF_EXIT_CODE_INIT_FAILED;
			}

			// Begin shutting down the application.
			pAppView->Shutdown();
			delete pAppView;
		}
		else
		{
			exitCode = DF_EXIT_CODE_APP_VIEW_NULL;
		}
	}
	else
	{
		exitCode = DF_EXIT_CODE_CREATE_CALLBACK_NULL;
	}

	// Shut down COM support.
	CoUninitialize();

	return (prv::fatalError ? DF_EXIT_CODE_FATAL_ERROR : exitCode);
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::RaiseFatalError()
{
	prv::fatalError = true;
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::WasFatalErrorRaised()
{
	return prv::fatalError;
}

//---------------------------------------------------------------------------------------------------------------------
