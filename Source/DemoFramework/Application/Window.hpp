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

#include "MouseButton.hpp"
#include "WindowEventListener.hpp"
#include "WindowStyle.hpp"

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework {
	class Window;
}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::Window
{
public:

	struct InitParams
	{
		DF_API static const InitParams Default; //!< Default initialization parameters object.

		const char* windowTitle; //!< Text to set in the window's title bar.

		uint32_t clientWidth; //!< Width of the window client area (does not include side borders).
		uint32_t clientHeight; //!< Height of the window client area (does not include bottom border or title bar).

		int32_t positionX; //!< Upper-left X coordinate of the window.
		int32_t positionY; //!< Upper-left Y coordinate of the window.

		WindowStyle style; //!< Style type of the window.
	};

	~Window();

	/*!
	 * Instantiate and initialize a native window.
	 *
	 * @param[in]  params          Window initialization parameters.
	 * @param[in]  pEventListener  Object to listen for window events; may be null if no event listening is required.
	 */
	static Window* Create(const InitParams& params, IWindowEventListener* pEventListener);

	//! Update the window, performing any necessary per-frame tasks such as window message processing.
	void Update();

	//! Make the window visible to the user.
	void Show();

	//! Hide the window.
	void Hide();

	//! Set the visibility of the mouse cursor within the window frame.
	void SetCursorVisibleState(bool visible);

	//! Set the position of the mouse cursor in the window's client area relative to the upper-left corner.
	void SetCursorPosition(int32_t clientX, int32_t clientY);

	//! Get the window handle object associated with the window.
	HWND GetWindowHandle() const;

	//! Get the current client width of the window.
	uint32_t GetClientWidth() const;

	//! Get the current client height of the window.
	uint32_t GetClientHeight() const;

	//! Get the current X coordinate of the window.
	int32_t GetWindowX() const;

	//! Get the current Y coordinate of the window.
	int32_t GetWindowY() const;

	//! Get the current X coordinate of the mouse cursor (relative to the window client).
	int32_t GetMouseX() const;

	//! Get the current Y coordinate of the mouse cursor (relative to the window client).
	int32_t GetMouseY() const;

	//! Get the mouse wheel delta for the current frame.
	float32_t GetMouseWheelDelta() const;

	//! Get the current pressed state of a mouse button.
	bool GetMouseButtonState(MouseButton button) const;

	//! Get the 'initialized' state of the window.
	bool IsInitialized() const;

	//! Get the 'visible' state of the window.
	bool IsVisible() const;


private:

	struct State
	{
		enum
		{
			Initialized = 0x01, //!< Window has been initialized.
			Visible = 0x02, //!< Window is currently visible.
		};
	};

	Window();

	HWND m_hWnd;

	IWindowEventListener* m_pEventListener;

	float32_t m_mouseWheelDelta;

	uint32_t m_stateFlags;

	uint32_t m_clientWidth;
	uint32_t m_clientHeight;

	int32_t m_windowX;
	int32_t m_windowY;

	int32_t m_mouseX;
	int32_t m_mouseY;

	bool m_mouseButton[int(MouseButton::Count)];
};

//---------------------------------------------------------------------------------------------------------------------

inline DemoFramework::Window::Window()
	: m_hWnd(nullptr)
	, m_pEventListener(nullptr)
	, m_mouseWheelDelta(0.0f)
	, m_stateFlags(State::Initialized)
	, m_clientWidth(0)
	, m_clientHeight(0)
	, m_windowX(0)
	, m_windowY(0)
	, m_mouseX(0)
	, m_mouseY(0)
	, m_mouseButton()
{
	memset(m_mouseButton, 0, sizeof(bool) * int(MouseButton::Count));
}

//---------------------------------------------------------------------------------------------------------------------

inline HWND DemoFramework::Window::GetWindowHandle() const
{
	return m_hWnd;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::Window::GetClientWidth() const
{
	return m_clientWidth;
}

//---------------------------------------------------------------------------------------------------------------------

inline uint32_t DemoFramework::Window::GetClientHeight() const
{
	return m_clientHeight;
}

//---------------------------------------------------------------------------------------------------------------------

inline int32_t DemoFramework::Window::GetWindowX() const
{
	return m_windowX;
}

//---------------------------------------------------------------------------------------------------------------------

inline int32_t DemoFramework::Window::GetWindowY() const
{
	return m_windowY;
}

//---------------------------------------------------------------------------------------------------------------------

inline int32_t DemoFramework::Window::GetMouseX() const
{
	return m_mouseX;
}

//---------------------------------------------------------------------------------------------------------------------

inline int32_t DemoFramework::Window::GetMouseY() const
{
	return m_mouseY;
}

//---------------------------------------------------------------------------------------------------------------------

inline float32_t DemoFramework::Window::GetMouseWheelDelta() const
{
	return m_mouseWheelDelta;
}

//---------------------------------------------------------------------------------------------------------------------

inline bool DemoFramework::Window::GetMouseButtonState(const MouseButton button) const
{
	return m_mouseButton[int(button)];
}

//---------------------------------------------------------------------------------------------------------------------

inline bool DemoFramework::Window::IsInitialized() const
{
	return (m_stateFlags & State::Initialized);
}

//---------------------------------------------------------------------------------------------------------------------

inline bool DemoFramework::Window::IsVisible() const
{
	return (m_stateFlags & State::Visible);
}

//---------------------------------------------------------------------------------------------------------------------
