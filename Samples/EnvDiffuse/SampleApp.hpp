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
#include <DemoFramework/Direct3D12/ReflectionProbe.hpp>
#include <DemoFramework/Direct3D12/WavefrontObj.hpp>

#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------------------------

#define APP_CONST_BUFFER_COUNT (DF_SWAP_CHAIN_BUFFER_MAX_COUNT + 1)

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

	bool prv_loadExternalFiles();
	bool prv_createEnvPipeline();
	bool prv_createObjPipeline();

	DemoFramework::Window* m_pWindow;

	DemoFramework::D3D12::RenderBase::Ptr m_renderBase;
	DemoFramework::D3D12::Gui::Ptr m_gui;

	DemoFramework::D3D12::RootSignature::Ptr m_envRootSig;
	DemoFramework::D3D12::PipelineState::Ptr m_envPipeline;

	DemoFramework::D3D12::RootSignature::Ptr m_objRootSig;
	DemoFramework::D3D12::PipelineState::Ptr m_objPipeline;

	DemoFramework::D3D12::Resource::Ptr m_envVertexBuffer;

	DemoFramework::D3D12::Resource::Ptr m_envConstBuffer[APP_CONST_BUFFER_COUNT];
	DemoFramework::D3D12::Resource::Ptr m_objConstBuffer[APP_CONST_BUFFER_COUNT];

	DemoFramework::D3D12::Descriptor m_envCbvDesc[APP_CONST_BUFFER_COUNT];
	DemoFramework::D3D12::Descriptor m_objCbvDesc[APP_CONST_BUFFER_COUNT];

	DemoFramework::D3D12::ReflectionProbe::Ptr m_reflectionProbe;
	DemoFramework::D3D12::WavefrontObj::Ptr m_object;

	DemoFramework::FrameTimer m_frameTimer;

	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projMatrix;
	DirectX::XMMATRIX m_viewInvMatrix;
	DirectX::XMMATRIX m_projInvMatrix;
	DirectX::XMMATRIX m_wvpMatrix;

	uint32_t m_constBufferIndex;

	bool m_resizeSwapChain;
};

//---------------------------------------------------------------------------------------------------------------------

inline SampleApp::SampleApp()
	: m_pWindow(nullptr)
	, m_renderBase()
	, m_gui()
	, m_envRootSig()
	, m_envPipeline()
	, m_objRootSig()
	, m_objPipeline()
	, m_envVertexBuffer()
	, m_envConstBuffer()
	, m_objConstBuffer()
	, m_envCbvDesc()
	, m_objCbvDesc()
	, m_reflectionProbe()
	, m_object()
	, m_frameTimer()
	, m_worldMatrix()
	, m_viewMatrix()
	, m_projMatrix()
	, m_viewInvMatrix()
	, m_projInvMatrix()
	, m_wvpMatrix()
	, m_constBufferIndex(0)
	, m_resizeSwapChain(false)
{
	m_frameTimer.SetFrameRateLocked(false);
}

//---------------------------------------------------------------------------------------------------------------------
