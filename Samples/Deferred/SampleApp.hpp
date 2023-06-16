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

#include "../Common/AppController.hpp"

#include <DemoFramework/Application/FrameTimer.hpp>

#include <DemoFramework/Direct3D12/RenderBase.hpp>
#include <DemoFramework/Direct3D12/Gui.hpp>

#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------------------------

class SampleApp
	: public IAppController
{
public:

	SampleApp(const SampleApp&) = delete;
	SampleApp(SampleApp&&) = delete;

	SampleApp& operator =(const SampleApp&) = delete;
	SampleApp& operator =(SampleApp&&) = delete;

	SampleApp();
	virtual ~SampleApp() {}

	virtual bool Initialize(DemoFramework::Window* pWindow) override;
	virtual bool Update() override;
	virtual void Render() override;
	virtual void Shutdown() override;

	virtual void OnWindowResized(DemoFramework::Window*, uint32_t /*previousWidth*/, uint32_t /*previousHeight*/) override;
	virtual void OnWindowMouseMove(DemoFramework::Window*, int32_t /*previousX*/, int32_t /*previousY*/) override;
	virtual void OnWindowMouseWheel(DemoFramework::Window*, float32_t /*wheelDelta*/) override;
	virtual void OnWindowMouseButtonPressed(DemoFramework::Window*, DemoFramework::MouseButton /*button*/) override;
	virtual void OnWindowMouseButtonReleased(DemoFramework::Window*, DemoFramework::MouseButton /*button*/) override;

	virtual const char* GetAppName() const override;
	virtual const char* GetLogFilename() const override;


private:

	DemoFramework::Window* m_pWindow;

	DemoFramework::D3D12::RenderBase::Ptr m_renderBase;
	DemoFramework::D3D12::Gui::Ptr m_gui;

	DemoFramework::FrameTimer m_frameTimer;

	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projMatrix;
	DirectX::XMMATRIX m_wvpMatrix;

	bool m_resizeSwapChain;
};

//---------------------------------------------------------------------------------------------------------------------

inline SampleApp::SampleApp()
	: m_pWindow()
	, m_renderBase()
	, m_gui()
	, m_frameTimer()
	, m_worldMatrix()
	, m_viewMatrix()
	, m_projMatrix()
	, m_wvpMatrix()
	, m_resizeSwapChain(false)
{
	m_frameTimer.SetFrameRateLocked(false);
}

//---------------------------------------------------------------------------------------------------------------------
