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

#include "AppController.hpp"

#include <DemoFramework/Application/AppView.hpp>
#include <DemoFramework/Application/Window.hpp>

#include <DemoFramework/Direct3D12/Types.hpp>

//---------------------------------------------------------------------------------------------------------------------

class CommonAppView
	: public DemoFramework::IAppView
{
public:

	CommonAppView(const CommonAppView&) = delete;
	CommonAppView(CommonAppView&&) = delete;

	CommonAppView& operator =(const CommonAppView&) = delete;
	CommonAppView& operator =(CommonAppView&&) = delete;

	CommonAppView() = delete;
	explicit CommonAppView(IAppController* pAppController);
	virtual ~CommonAppView() {}

	virtual bool Initialize() override;
	virtual bool MainLoopUpdate() override;
	virtual void Shutdown() override;


protected:

	DemoFramework::Window* m_pWindow;
	IAppController* m_pAppController;
};

//---------------------------------------------------------------------------------------------------------------------

inline CommonAppView::CommonAppView(IAppController* pAppController)
	: m_pWindow(nullptr)
	, m_pAppController(pAppController)
{
}

//---------------------------------------------------------------------------------------------------------------------
