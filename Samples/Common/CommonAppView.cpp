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

#include "CommonAppView.hpp"

#include <DemoFramework/Application/Log.hpp>

#include <stdio.h>

//---------------------------------------------------------------------------------------------------------------------

bool CommonAppView::Initialize()
{
	using namespace DemoFramework;

	Log::OpenFile(m_pAppController ? m_pAppController->GetLogFilename() : nullptr);

	LOG_WRITE("Initializing application ...");

	Window::InitParams windowInit = Window::InitParams::Default;
	windowInit.windowTitle = m_pAppController ? m_pAppController->GetAppName() : nullptr;
	windowInit.style = WindowStyle::Centered;

	// Create the application window.
	m_pWindow = Window::Create(windowInit, m_pAppController);
	if(!m_pWindow)
	{
		return false;
	}

	// Initialize the application controller.
	if(m_pAppController && !m_pAppController->Initialize(m_pWindow))
	{
		return false;
	}

	// Make the window visible.
	m_pWindow->Show();

	LOG_WRITE("... initialization successful");

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool CommonAppView::MainLoopUpdate()
{
	m_pWindow->Update();

	if(!m_pWindow->IsInitialized())
	{
		// Stop the application when the window has been closed.
		return false;
	}

	if(m_pAppController)
	{
		if(!m_pAppController->Update())
		{
			return false;
		}

		m_pAppController->Render();
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void CommonAppView::Shutdown()
{
	using namespace DemoFramework;

	LOG_WRITE("Shutting down ...");

	if(m_pAppController)
	{
		m_pAppController->Shutdown();

		delete m_pAppController;
		m_pAppController = nullptr;
	}

	if(m_pWindow)
	{
		delete m_pWindow;
		m_pWindow = nullptr;
	}

	LOG_WRITE("... shutdown complete");
	Log::CloseFile();
}

//---------------------------------------------------------------------------------------------------------------------
