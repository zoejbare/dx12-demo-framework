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

#include "../BuildSetup.h"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework
{
	class FrameTimer;
}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::FrameTimer
{
public:

	FrameTimer();

	void Update();

	float64_t GetDeltaTime() const;
	float64_t GetTotalTime() const;
	float64_t GetTargetFps() const;
	float64_t GetFps() const;
	bool IsFrameRateLocked() const;

	void SetTargetFps(float64_t value);
	void SetFrameRateLocked(bool value);


private:

	float64_t m_deltaTime;
	float64_t m_totalTime;
	float64_t m_targetFps;
	float64_t m_currentFps;

	LARGE_INTEGER m_frequency;
	LARGE_INTEGER m_previousTime;

	bool m_lockFrameRate;
};

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::FrameTimer::FrameTimer()
	: m_deltaTime(0.0)
	, m_totalTime(0.0)
	, m_targetFps(60.0)
	, m_currentFps(0.0)
	, m_frequency()
	, m_previousTime()
	, m_lockFrameRate(true)
{
	QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_previousTime);
}

//---------------------------------------------------------------------------------------------------------------------

inline float64_t DemoFramework::FrameTimer::GetDeltaTime() const
{
	return m_deltaTime;
}

//---------------------------------------------------------------------------------------------------------------------

inline float64_t DemoFramework::FrameTimer::GetTotalTime() const
{
	return m_totalTime;
}

//---------------------------------------------------------------------------------------------------------------------

inline float64_t DemoFramework::FrameTimer::GetTargetFps() const
{
	return m_targetFps;
}

//---------------------------------------------------------------------------------------------------------------------

inline float64_t DemoFramework::FrameTimer::GetFps() const
{
	return m_currentFps;
}

//---------------------------------------------------------------------------------------------------------------------

inline bool DemoFramework::FrameTimer::IsFrameRateLocked() const
{
	return m_lockFrameRate;
}

//---------------------------------------------------------------------------------------------------------------------

inline void DemoFramework::FrameTimer::SetTargetFps(const float64_t value)
{
	m_targetFps = value;
}

//---------------------------------------------------------------------------------------------------------------------

inline void DemoFramework::FrameTimer::SetFrameRateLocked(const bool value)
{
	m_lockFrameRate = value;
}

//---------------------------------------------------------------------------------------------------------------------
