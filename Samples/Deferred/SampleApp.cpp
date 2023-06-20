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

#include "SampleApp.hpp"

#include <DemoFramework/Application/Log.hpp>
#include <DemoFramework/Application/Window.hpp>

#include <DemoFramework/Direct3D12/LowLevel/CommandAllocator.hpp>
#include <DemoFramework/Direct3D12/LowLevel/DescriptorHeap.hpp>
#include <DemoFramework/Direct3D12/LowLevel/GraphicsCommandList.hpp>
#include <DemoFramework/Direct3D12/LowLevel/PipelineState.hpp>
#include <DemoFramework/Direct3D12/LowLevel/Resource.hpp>
#include <DemoFramework/Direct3D12/LowLevel/RootSignature.hpp>

#include <DemoFramework/Direct3D12/Shader.hpp>
#include <DemoFramework/Direct3D12/Sync.hpp>

#include <imgui.h>

#include <math.h>

//---------------------------------------------------------------------------------------------------------------------

#define APP_NAME "Deferred Rendering Sample"
#define LOG_FILE "deferred-sample.log"

#define APP_BACK_BUFFER_COUNT   2
#define APP_BACK_BUFFER_FORMAT  DXGI_FORMAT_R8G8B8A8_UNORM
#define APP_DEPTH_BUFFER_FORMAT DXGI_FORMAT_D32_FLOAT

#define M_PI  3.1415926535897932384626433832795f
#define M_TAU (M_PI * 2.0f)

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Initialize(DemoFramework::Window* const pWindow)
{
	using namespace DemoFramework;

	m_pWindow = pWindow;

	HWND hWnd = m_pWindow->GetWindowHandle();

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	LOG_WRITE("Initializing base render resources ...");

	D3D12::RenderConfig renderConfig = D3D12::RenderConfig::Invalid;
	renderConfig.backBufferWidth = clientWidth;
	renderConfig.backBufferHeight = clientHeight;
	renderConfig.backBufferCount = APP_BACK_BUFFER_COUNT;
	renderConfig.cbvSrvUavDescCount = 100;
	renderConfig.rtvDescCount = APP_BACK_BUFFER_COUNT;
	renderConfig.dsvDescCount = 1;
	renderConfig.backBufferFormat = APP_BACK_BUFFER_FORMAT;
	renderConfig.depthFormat = APP_DEPTH_BUFFER_FORMAT;

	// Initialize the common rendering resources.
	m_renderBase = D3D12::RenderBase::Create(hWnd, renderConfig);
	if(!m_renderBase)
	{
		return false;
	}

	LOG_WRITE("Initializing GUI resources ...");

	// Initialize the on-screen GUI.
	m_gui = D3D12::Gui::Create(m_renderBase->GetDevice(), APP_NAME, APP_BACK_BUFFER_COUNT, APP_BACK_BUFFER_FORMAT);
	if(!m_gui)
	{
		return false;
	}

	// Set the size of the GUI display area.
	m_gui->SetDisplaySize(pWindow->GetClientWidth(), pWindow->GetClientHeight());

	// Initialize the frame timer at the end so the initial timestamp is not
	// influenced by the time it takes to create the application resources.
	m_frameTimer.Initialize();

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool SampleApp::Update()
{
	using namespace DemoFramework;
	using namespace DirectX;

	if(m_resizeSwapChain)
	{
		const bool resizeSwapChainResult = m_renderBase->ResizeSwapChain();
		assert(resizeSwapChainResult == true); (void) resizeSwapChainResult;

		m_resizeSwapChain = false;
	}

	m_frameTimer.Update();

	m_gui->Update(
		m_frameTimer.GetDeltaTime(),
		[](ImGuiContext* const pGuiContext)
		{
			// Always set the ImGui context before calling any other ImGui functions.
			ImGui::SetCurrentContext(pGuiContext);

			// Do custom GUI drawing code here.
		}
	);

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Render()
{
	using namespace DemoFramework;

	const uint32_t clientWidth = m_pWindow->GetClientWidth();
	const uint32_t clientHeight = m_pWindow->GetClientHeight();

	m_renderBase->BeginFrame();
	m_renderBase->SetBackBufferAsRenderTarget();

	// The swap chain buffer index is updated during the call to BeginFrame(),
	// so we need to wait until *after* that to get the current command list.
	ID3D12GraphicsCommandList* const pCmdList = m_renderBase->GetDrawContext()->GetCmdList().Get();

	// Set the screen viewport.
	const D3D12_VIEWPORT viewport =
	{
		0.0f,                    // FLOAT TopLeftX
		0.0f,                    // FLOAT TopLeftY
		float32_t(clientWidth),  // FLOAT Width
		float32_t(clientHeight), // FLOAT Height
		0.0f,                    // FLOAT MinDepth
		1.0f,                    // FLOAT MaxDepth
	};
	pCmdList->RSSetViewports(1, &viewport);

	// Set the scissor region of the viewport.
	const D3D12_RECT scissorRect =
	{
		0,                  // LONG    left
		0,                  // LONG    top
		LONG(clientWidth),  // LONG    right
		LONG(clientHeight), // LONG    bottom
	};
	pCmdList->RSSetScissorRects(1, &scissorRect);

	// Draw the GUI.
	m_gui->Render(pCmdList);

	m_renderBase->EndFrame(true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::Shutdown()
{
	// Do explicit shutdown tasks here.
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowResized(DemoFramework::Window* const pWindow, const uint32_t previousWidth, const uint32_t previousHeight)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousWidth);
	DF_UNUSED(previousHeight);

	m_gui->SetDisplaySize(pWindow->GetClientWidth(), pWindow->GetClientHeight());

	m_resizeSwapChain = true;
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseMove(DemoFramework::Window* const pWindow, const int32_t previousX, const int32_t previousY)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(previousX);
	DF_UNUSED(previousY);

	m_gui->SetMousePosition(pWindow->GetMouseX(), pWindow->GetMouseY());
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseWheel(DemoFramework::Window* const pWindow, const float32_t wheelDelta)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(wheelDelta);

	m_gui->SetMouseWheelDelta(wheelDelta);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseButtonPressed(DemoFramework::Window* const pWindow, const DemoFramework::MouseButton button)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(button);

	using namespace DemoFramework;

	uint32_t buttonIndex = UINT_MAX;
	switch(button)
	{
		case MouseButton::Left:   buttonIndex = 0; break;
		case MouseButton::Right:  buttonIndex = 1; break;
		case MouseButton::Middle: buttonIndex = 2; break;
		case MouseButton::X1:     buttonIndex = 3; break;
		case MouseButton::X2:     buttonIndex = 4; break;

		default:
			assert(false);
			break;
	}

	m_gui->SetMouseButtonState(buttonIndex, true);
}

//---------------------------------------------------------------------------------------------------------------------

void SampleApp::OnWindowMouseButtonReleased(DemoFramework::Window* const pWindow, const DemoFramework::MouseButton button)
{
	DF_UNUSED(pWindow);
	DF_UNUSED(button);

	using namespace DemoFramework;

	uint32_t buttonIndex = UINT_MAX;
	switch(button)
	{
		case MouseButton::Left:   buttonIndex = 0; break;
		case MouseButton::Right:  buttonIndex = 1; break;
		case MouseButton::Middle: buttonIndex = 2; break;
		case MouseButton::X1:     buttonIndex = 3; break;
		case MouseButton::X2:     buttonIndex = 4; break;

		default:
			assert(false);
			break;
	}

	m_gui->SetMouseButtonState(buttonIndex, false);
}

//---------------------------------------------------------------------------------------------------------------------

const char* SampleApp::GetAppName() const
{
	return APP_NAME;
}

//---------------------------------------------------------------------------------------------------------------------

const char* SampleApp::GetLogFilename() const
{
	return LOG_FILE;
}

//---------------------------------------------------------------------------------------------------------------------
