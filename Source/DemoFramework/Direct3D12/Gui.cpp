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

#include "Gui.hpp"

#include "LowLevel/DescriptorHeap.hpp"

#include "../Application/Log.hpp"

#include <imgui.h>
#include <implot.h>

#include <backends/imgui_impl_dx12.h>

#include <DirectXMath.h>

//---------------------------------------------------------------------------------------------------------------------

constexpr DXGI_SAMPLE_DESC defaultSampleDesc =
{
	1, // UINT Count
	0, // UINT Quality
};

constexpr D3D12_RANGE cpuAccessDisabledRange =
{
	0, // SIZE_T Begin
	0, // SIZE_T End
};

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Gui::~Gui()
{
	if(m_pGuiContext)
	{
		ImGui::SetCurrentContext(m_pGuiContext);
		ImPlot::SetCurrentContext(m_pPlotContext);

		// Release all internal GUI resources.
		ImGui_ImplDX12_Shutdown();

		ImPlot::DestroyContext();
		ImGui::DestroyContext();
	}
}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::D3D12::Gui::Ptr DemoFramework::D3D12::Gui::Create(
	const Device::Ptr& device,
	const char* const demoName,
	const uint32_t bufferCount,
	const DXGI_FORMAT renderTargetFormat)
{
	if(!device
		|| !demoName
		|| demoName[0] == '\0'
		|| bufferCount == 0
		|| bufferCount > DF_SWAP_CHAIN_BUFFER_MAX_COUNT
		|| renderTargetFormat == DXGI_FORMAT_UNKNOWN)
	{
		LOG_ERROR("Invalid parameter");
		return Ptr();
	}

	Ptr output = std::make_shared<Gui>();

	constexpr D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc =
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,    // D3D12_DESCRIPTOR_HEAP_TYPE Type
		1,                                         // UINT NumDescriptors
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, // D3D12_DESCRIPTOR_HEAP_FLAGS Flags
		0,                                         // UINT NodeMask
	};

	LOG_WRITE("Creating ImGui font SRV descriptor heap ...");

	// Create the font SRV heap.
	output->m_fontSrvHeap = CreateDescriptorHeap(device, descHeapDesc);
	if(!output->m_fontSrvHeap)
	{
		return Ptr();
	}

	LOG_WRITE("Creating ImGui context ...");

	// Create the ImGui context.
	output->m_pGuiContext = ImGui::CreateContext();
	if(!output->m_pGuiContext)
	{
		LOG_ERROR("Failed to create ImGui context");
		return Ptr();
	}

	LOG_WRITE("Creating ImPlot context ...");

	// Create the ImPlot context.
	output->m_pPlotContext = ImPlot::CreateContext();
	if(!output->m_pPlotContext)
	{
		LOG_ERROR("Failed to create ImPlot context");
		return Ptr();
	}

	// Update ImGui's view of the current context so we can use the one we just created.
	ImGui::SetCurrentContext(output->m_pGuiContext);
	ImPlot::SetCurrentContext(output->m_pPlotContext);

	// Configure the new ImGui context.
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

	// Use the default dark color scheme.
	ImGui::StyleColorsDark();
	ImPlot::StyleColorsDark();

	// Initialize the GUI's DirectX 12 backend implementation.
	//
	// NOTE: The use of an ImGui backend implementation is not required,
	//       but for use in a demo application, it removes the need to
	//       setup a lot of boilerplate code.
	ImGui_ImplDX12_Init(
		device.Get(),
		bufferCount,
		renderTargetFormat,
		output->m_fontSrvHeap.Get(),
		output->m_fontSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		output->m_fontSrvHeap->GetGPUDescriptorHandleForHeapStart());

	snprintf(output->m_demoName, GUI_NAME_BUFFER_SIZE, "%s", demoName);

	output->m_initialized = true;

	return output;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Gui::Update(const float64_t deltaTime, CustomGuiDrawFn customGuiDraw)
{
	if(m_initialized)
	{
		ImGui::SetCurrentContext(m_pGuiContext);

		// Tell the GUI system that a new frame has now begun.
		ImGui_ImplDX12_NewFrame();
		ImGui::NewFrame();

		ImGuiIO& io = ImGui::GetIO();

		// This is tightly calibrated against the number of plot samples to make the line graph look correct.
		// A longer interval time requires fewer samples or slowing down the scroll speed of the graph.
		constexpr float64_t updateInterval = 0.2;

		m_totalTime += deltaTime;
		m_updateTimer += deltaTime;

		if(m_deltaTime.count < GUI_TIME_SAMPLE_MAX_COUNT)
		{
			m_deltaTime.samples[m_deltaTime.count] = deltaTime;
			++m_deltaTime.count;
		}
		else
		{
			m_deltaTime.samples[m_deltaTime.offset] = deltaTime;
			m_deltaTime.offset = (m_deltaTime.offset + 1) % GUI_TIME_SAMPLE_MAX_COUNT;
		}

		if(m_updateTimer >= updateInterval)
		{
			float64_t totalDeltaAccum = 0.0;
			for(size_t i = 0; i < m_deltaTime.count; ++i)
			{
				totalDeltaAccum += m_deltaTime.samples[i];
			}

			m_deltaTime.avg = totalDeltaAccum / float64_t(m_deltaTime.count);
			m_updateTimer -= updateInterval;

			if(m_totalTime > 1.5)
			{
				FrameTimePlotData plotData;
				plotData.time = m_totalTime;
				plotData.ms = m_deltaTime.avg;

				if(m_frameTimePlot.count < GUI_PLOT_SAMPLE_MAX_COUNT)
				{
					m_frameTimePlot.samples[m_frameTimePlot.count] = plotData;
					++m_frameTimePlot.count;
				}
				else
				{
					m_frameTimePlot.samples[m_frameTimePlot.offset] = plotData;
					m_frameTimePlot.offset = (m_frameTimePlot.offset + 1) % GUI_PLOT_SAMPLE_MAX_COUNT;
				}
			}
		}

        constexpr float32_t edgePadding = 10.0f;
        constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoMove;

        const ImGuiViewport* const pViewport = ImGui::GetMainViewport();

        const ImVec2 workAreaPos = pViewport->WorkPos;
        const ImVec2 workAreaSize = pViewport->WorkSize;

        ImVec2 windowPos;
		ImVec2 windowPivot;

        windowPos.x = (workAreaPos.x + workAreaSize.x - edgePadding);
        windowPos.y = (workAreaPos.y + workAreaSize.y - edgePadding);
        windowPivot.x = 1.0f;
        windowPivot.y = 1.0f;

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);
        ImGui::SetNextWindowBgAlpha(0.75f);

		bool showOverlay = true;
        if(ImGui::Begin("Metrics Overlay", &showOverlay, window_flags))
        {
			constexpr float64_t minuteConv = 1.0 / 60.0;
			constexpr float64_t hourConv = 1.0 / 3600.0;

			const float64_t deltaTimeMs = m_deltaTime.avg * 1000.0;
			const float64_t fps = 1.0 / m_deltaTime.avg;

			const float64_t upTimeSeconds = fmod(m_totalTime, 60.0);
			const float64_t upTimeMinutes = floor(fmod(m_totalTime * minuteConv, 60.0));
			const float64_t upTimeHours = floor(m_totalTime * hourConv);

			ImGui::Text(m_demoName);
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::Text("Up time: %0.0f:%02.0f:%06.3f", upTimeHours, upTimeMinutes, upTimeSeconds);
            ImGui::Text("Delta:   %.3f ms (%.1f FPS)", deltaTimeMs, fps);

			constexpr float64_t historyLength = 10.0;
			constexpr ImPlotFlags plotFlags = ImPlotFlags_CanvasOnly
				| ImPlotFlags_NoInputs
				| ImPlotFlags_NoFrame
				| ImPlotFlags_NoChild;

			if(ImPlot::BeginPlot("##fps_plot", ImVec2(-1.0f, 40.0f), plotFlags))
			{
				const char* const plotGraphName = "##fps_plot_line";

				const float64_t minX = m_totalTime - historyLength;
				const float64_t maxX = m_totalTime - 0.25;

				auto plotDataGetter = [](const int index, void* const pData) -> ImPlotPoint
				{
					const FrameTimePlot& plot = *reinterpret_cast<const FrameTimePlot*>(pData);
					const FrameTimePlotData& plotData = plot.samples[(plot.offset + size_t(index)) % plot.count];

					return ImPlotPoint(plotData.time, plotData.ms);
				};

				ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_AutoFit);
				ImPlot::SetupAxisLimits(ImAxis_X1, minX, maxX, ImGuiCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0f, 200.0);

				ImPlot::SetNextLineStyle();
				ImPlot::PlotLineG(plotGraphName, plotDataGetter, &m_frameTimePlot, int(m_frameTimePlot.count));

				ImPlot::EndPlot();
			}

			ImGui::End();
        }

		// Do any custom GUI work needed by the calling application.
		if(customGuiDraw)
		{
			customGuiDraw(m_pGuiContext);
		}

		// Reset the mouse wheel delta.
		io.MouseWheel = 0.0f;
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Gui::Render(const GraphicsCommandList::Ptr& cmdList)
{
	using namespace DirectX;

	if(m_initialized)
	{
		ImGui::SetCurrentContext(m_pGuiContext);

		// Tell the ImGui context to get its software render buffers ready for us to draw them.
		ImGui::Render();

		ImDrawData* const pDrawData = ImGui::GetDrawData();

		// Set the font SRV descriptor heap.
		ID3D12DescriptorHeap* const pDescHeaps[] =
		{
			m_fontSrvHeap.Get(),
		};
		cmdList->SetDescriptorHeaps(_countof(pDescHeaps), pDescHeaps);

		// Issue the GUI draw calls to DirectX.
		ImGui_ImplDX12_RenderDrawData(pDrawData, cmdList.Get());
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Gui::SetDisplaySize(const uint32_t width, const uint32_t height)
{
	if(m_initialized)
	{
		ImGui::SetCurrentContext(m_pGuiContext);

		ImGuiIO& io = ImGui::GetIO();

		io.DisplaySize.x = float32_t(width);
		io.DisplaySize.y = float32_t(height);
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Gui::SetMousePosition(const int32_t positionX, const int32_t positionY)
{
	if(m_initialized)
	{
		ImGui::SetCurrentContext(m_pGuiContext);

		ImGuiIO& io = ImGui::GetIO();

		io.MousePos.x = float32_t(positionX);
		io.MousePos.y = float32_t(positionY);
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Gui::SetMouseWheelDelta(const float32_t wheelDelta)
{
	if(m_initialized)
	{
		ImGui::SetCurrentContext(m_pGuiContext);

		ImGuiIO& io = ImGui::GetIO();

		io.MouseWheel = wheelDelta;
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::D3D12::Gui::SetMouseButtonState(const uint32_t buttonIndex, const bool isDown)
{
	if(m_initialized)
	{
		ImGui::SetCurrentContext(m_pGuiContext);

		ImGuiIO& io = ImGui::GetIO();

		io.MouseDown[buttonIndex] = isDown;
	}
}

//---------------------------------------------------------------------------------------------------------------------
