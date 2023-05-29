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

#include "MouseButton.hpp"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework
{
	class IWindowEventListener;
	class BaseWindowEventListener;

	class Window;
}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::IWindowEventListener
{
public:

	virtual ~IWindowEventListener() {}

	/*!
	 * Event fired when the window has been instructed to close.
	 *
	 * This event can be canceled by setting the @c cancel parameter to @c true; doing this will prevent the window from closing.
	 */
	virtual void OnWindowClose(Window*, bool& /*cancel*/) = 0;

	/*!
	 * Event fired when the window has been moved.
	 *
	 * This event will actively fire as a window is being moved.
	 */
	virtual void OnWindowMoved(Window*, int32_t /*previousX*/, int32_t /*previousY*/) = 0;

	/*!
	 * Event fired when the window has been resized.
	 *
	 * This event will fire when the window has finished resizing.
	 */
	virtual void OnWindowResized(Window*, uint32_t /*previousWidth*/, uint32_t /*previousHeight*/) = 0;

	//! Event fired when the window has gained foreground focus.
	virtual void OnWindowGainedFocus(Window*) = 0;

	//! Event fired when the window has lost foreground focus.
	virtual void OnWindowLostFocus(Window*) = 0;

	//! Event fired when the mouse cursor has moved within the window client area.
	virtual void OnWindowMouseMove(Window*, int32_t /*previousX*/, int32_t /*previousY*/) = 0;

	//! Event fired when the mouse wheel has moved within the window client area.
	virtual void OnWindowMouseWheel(Window*, float32_t /*wheelDelta*/) = 0;

	//! Event fired when a mouse button has been pressed within the window client area.
	virtual void OnWindowMouseButtonPressed(Window*, MouseButton /*button*/) = 0;

	//! Event fired when a mouse button has been released.
	virtual void OnWindowMouseButtonReleased(Window*, MouseButton /*button*/) = 0;
};

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::BaseWindowEventListener
	: public DemoFramework::IWindowEventListener
{
public:

	virtual ~BaseWindowEventListener() {}

	virtual void OnWindowClose(Window*, bool& /*cancel*/) override {}
	virtual void OnWindowMoved(Window*, int32_t /*previousX*/, int32_t /*previousY*/) override {}
	virtual void OnWindowResized(Window*, uint32_t /*previousWidth*/, uint32_t /*previousHeight*/) override {}
	virtual void OnWindowGainedFocus(Window*) override {}
	virtual void OnWindowLostFocus(Window*) override {}
	virtual void OnWindowMouseMove(Window*, int32_t /*previousX*/, int32_t /*previousY*/) override {}
	virtual void OnWindowMouseWheel(Window*, float32_t /*wheelDelta*/) override {}
	virtual void OnWindowMouseButtonPressed(Window*, MouseButton /*button*/) override {}
	virtual void OnWindowMouseButtonReleased(Window*, MouseButton /*button*/) override {}
};

//---------------------------------------------------------------------------------------------------------------------
