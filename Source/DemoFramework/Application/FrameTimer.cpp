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

#include "FrameTimer.hpp"

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::FrameTimer::Update()
{
	const float64_t desiredDeltaTime = m_lockFrameRate
		? (1.0 / m_targetFps)
		: -1.0f;

	LARGE_INTEGER currentTime;
	float64_t deltaTime = 0.0;

	do
	{
		QueryPerformanceCounter(&currentTime);
		deltaTime = float64_t(currentTime.QuadPart - m_previousTime.QuadPart) / float64_t(m_frequency.QuadPart);
	} while(desiredDeltaTime > 0.0 && deltaTime < desiredDeltaTime);

	m_previousTime = currentTime;
	m_deltaTime = deltaTime;

	m_totalTime += m_deltaTime;
	m_currentFps = (m_deltaTime > FLT_EPSILON)
		? (1.0 / m_deltaTime)
		: 0.0;
}

//---------------------------------------------------------------------------------------------------------------------
