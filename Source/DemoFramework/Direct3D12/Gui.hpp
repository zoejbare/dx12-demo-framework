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

#include "LowLevel/Types.hpp"

#include <functional>

//---------------------------------------------------------------------------------------------------------------------

#define GUI_TIME_SAMPLE_MAX_COUNT 100
#define GUI_PLOT_SAMPLE_MAX_COUNT 60

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace D3D12 {
	class Gui;
}}

struct ImGuiContext;
struct ImPlotContext;

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::D3D12::Gui
{
public:

	typedef std::function<void()> CustomGuiDrawFn;

	Gui();
	Gui(const Gui&) = delete;
	Gui(Gui&&) = delete;
	~Gui();

	Gui& operator =(const Gui&) = delete;
	Gui& operator =(Gui&&) = delete;

	bool Initialize(const DevicePtr& pDevice, uint32_t bufferCount, DXGI_FORMAT renderTargetFormat);
	void Update(float64_t deltaTime, CustomGuiDrawFn customGuiDraw = nullptr);
	void Render(const GraphicsCommandListPtr& pCmdList);

	void SetDisplaySize(uint32_t width, uint32_t height);
	void SetMousePosition(int32_t positionX, int32_t positionY);
	void SetMouseWheelDelta(float32_t wheelDelta);
	void SetMouseButtonState(uint32_t buttonIndex, bool isDown);


private:

	struct FrameTimePlotData
	{
		float64_t time;
		float64_t ms;
	};

	struct FrameTimePlot
	{
		FrameTimePlotData samples[GUI_PLOT_SAMPLE_MAX_COUNT];

		size_t count;
		size_t offset;
	};

	struct DeltaTime
	{
		float64_t samples[GUI_TIME_SAMPLE_MAX_COUNT];
		float64_t avg;

		size_t count;
		size_t offset;
	};

	FrameTimePlot m_frameTimePlot;
	DeltaTime m_deltaTime;

	DescriptorHeapPtr m_pFontSrvHeap;

	ImGuiContext* m_pGuiContext;
	ImPlotContext* m_pPlotContext;

	float64_t m_totalTime;
	float64_t m_updateTimer;
};

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::D3D12::Gui::Gui()
	: m_frameTimePlot()
	, m_deltaTime()
	, m_pFontSrvHeap()
	, m_pGuiContext(nullptr)
	, m_pPlotContext(nullptr)
	, m_totalTime(0.0)
	, m_updateTimer(0.0)
{
}

//---------------------------------------------------------------------------------------------------------------------
