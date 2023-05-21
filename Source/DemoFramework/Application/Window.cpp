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

#include "Window.hpp"

#include "Log.hpp"

#include <windowsx.h>

#include <unordered_map>

//---------------------------------------------------------------------------------------------------------------------

#define DF_WINDOW_CLASS_NAME "DemoFramework"

//---------------------------------------------------------------------------------------------------------------------

const DemoFramework::Window::InitParams DemoFramework::Window::InitParams::Default =
{
	"DemoFramework",       // const char* windowTitle
	1280,                  // uint32_t clientWidth
	720,                   // uint32_t clientHeight
	10,                    // int32_t positionX
	10,                    // int32_t positionY
	WindowStyle::Standard, // WindowStyle style
};

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::Window::~Window()
{
	if(m_stateFlags & State::Initialized)
	{
		// This will automatically call the WM_DESTROY event which will finish cleaning things up.
		DestroyWindow(m_hWnd);
	}
}

//---------------------------------------------------------------------------------------------------------------------

DemoFramework::Window* DemoFramework::Window::Create(const InitParams& params, IWindowEventListener* const pEventListener)
{
	static BaseWindowEventListener defaultEventListener;

	auto wndProc = [](HWND hWnd, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> LRESULT
	{
		Window* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

		if(pWindow)
		{
			IWindowEventListener* const pEventListener = pWindow->m_pEventListener;

			auto handleMouseButtonPressed = [&pWindow, &pEventListener](const MouseButton button) -> bool
			{
				bool& buttonState = pWindow->m_mouseButton[int(button)];

				if(!buttonState)
				{
					buttonState = true;

					pEventListener->OnWindowMouseButtonPressed(pWindow, button);
					return true;
				}

				return false;
			};

			auto handleMouseButtonReleased = [&pWindow, &pEventListener](const MouseButton button) -> bool
			{
				bool& buttonState = pWindow->m_mouseButton[int(button)];

				if(buttonState)
				{
					buttonState = false;

					pEventListener->OnWindowMouseButtonReleased(pWindow, button);
					return true;
				}

				return false;
			};

			switch(msg)
			{
				// Window focus event.
				case WM_ACTIVATE:
				{
					const uint16_t focusState = LOWORD(wparam);

					if(focusState != WA_INACTIVE)
					{
						pEventListener->OnWindowGainedFocus(pWindow);
					}
					else if(focusState == WA_INACTIVE)
					{
						pEventListener->OnWindowLostFocus(pWindow);
					}
					return 0;
				}

				// Window close request event.
				case WM_CLOSE:
				{
					bool cancel = false;

					pEventListener->OnWindowClose(pWindow, cancel);

					if(!cancel)
					{
						DestroyWindow(hWnd);
					}

					return 0;
				}

				// Window destroyed event.
				case WM_DESTROY:
				{
					pWindow->m_stateFlags &= ~State::Initialized;

					UnregisterClassA(DF_WINDOW_CLASS_NAME, nullptr);
					return 0;
				}

				// Window moving/moved event.
				case WM_MOVE:
				case WM_MOVING:
				{
					RECT windowRect;
					GetWindowRect(hWnd, &windowRect);

					const int32_t newWindowX = int32_t(windowRect.left);
					const int32_t newWindowY = int32_t(windowRect.top);

					// Ignore any resize events where the window position has not changed.
					if((pWindow->m_windowX == newWindowX)
						&& (pWindow->m_windowY == newWindowY))
					{
						break;
					}

					const int32_t oldWindowX = pWindow->m_windowX;
					const int32_t oldWindowY = pWindow->m_windowY;

					pWindow->m_windowX = newWindowX;
					pWindow->m_windowY = newWindowY;

					pEventListener->OnWindowMoved(pWindow, oldWindowX, oldWindowY);

					return 0;
				}

				// Window resized/resizing events.
				case WM_EXITSIZEMOVE:
				case WM_SIZE:
				{
					if(msg == WM_SIZE)
					{
						const uint16_t windowState = LOWORD(wparam);

						if((windowState != SIZE_MAXIMIZED) && (windowState != SIZE_RESTORED))
						{
							break;
						}
					}

					RECT clientRect;
					GetClientRect(hWnd, &clientRect);

					const uint32_t newClientWidth = uint32_t(clientRect.right - clientRect.left);
					const uint32_t newClientHeight = uint32_t(clientRect.bottom - clientRect.top);

					// Ignore any resize events where the window position has not changed.
					if((pWindow->m_clientWidth == newClientWidth)
						&& (pWindow->m_clientHeight == newClientHeight))
					{
						break;
					}

					const int32_t oldClientWidth = pWindow->m_clientWidth;
					const int32_t oldClientHeight = pWindow->m_clientHeight;

					pWindow->m_clientWidth = newClientWidth;
					pWindow->m_clientHeight = newClientHeight;

					pEventListener->OnWindowResized(pWindow, oldClientWidth, oldClientHeight);

					return 0;
				}

				case WM_MOUSEMOVE:
				{
					const int32_t newMouseX = GET_X_LPARAM(lparam);
					const int32_t newMouseY = GET_Y_LPARAM(lparam);

					// Ignore any resize events where the window position has not changed.
					if((pWindow->m_mouseX == newMouseX)
						&& (pWindow->m_mouseY == newMouseY))
					{
						break;
					}

					const int32_t oldMouseX = pWindow->m_mouseX;
					const int32_t oldMouseY = pWindow->m_mouseY;

					pWindow->m_mouseX = newMouseX;
					pWindow->m_mouseY = newMouseY;

					pEventListener->OnWindowMouseMove(pWindow, oldMouseX, oldMouseY);

					return 0;
				}

				case WM_LBUTTONDOWN:
					if(handleMouseButtonPressed(MouseButton::Left))
					{
						return 0;
					}
					break;

				case WM_LBUTTONUP:
				case WM_NCLBUTTONUP:
					if(handleMouseButtonReleased(MouseButton::Left))
					{
						return 0;
					}
					break;

				case WM_RBUTTONDOWN:
					if(handleMouseButtonPressed(MouseButton::Right))
					{
						return 0;
					}
					break;

				case WM_RBUTTONUP:
				case WM_NCRBUTTONUP:
					if(handleMouseButtonReleased(MouseButton::Right))
					{
						return 0;
					}
					break;

				case WM_MBUTTONDOWN:
					if(handleMouseButtonPressed(MouseButton::Middle))
					{
						return 0;
					}
					break;

				case WM_MBUTTONUP:
				case WM_NCMBUTTONUP:
					if(handleMouseButtonReleased(MouseButton::Middle))
					{
						return 0;
					}
					break;

				case WM_XBUTTONDOWN:
				{
					const MouseButton button = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
						? MouseButton::X1
						: MouseButton::X2;

					if(handleMouseButtonPressed(button))
					{
						return 0;
					}

					break;
				}

				case WM_XBUTTONUP:
				case WM_NCXBUTTONUP:
				{
					const MouseButton button = (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
						? MouseButton::X1
						: MouseButton::X2;

					if(handleMouseButtonReleased(button))
					{
						return 0;
					}

					break;
				}

				case WM_MOUSEWHEEL:
				{
					const float32_t delta = float32_t(GET_WHEEL_DELTA_WPARAM(wparam)) / float32_t(WHEEL_DELTA);

					pWindow->m_mouseWheelDelta = delta;

					pEventListener->OnWindowMouseWheel(pWindow, delta);
					return 0;
				}

				default:
					break;
			}
		}

		// Catch all for any events we haven't processed.
		return DefWindowProcA(hWnd, msg, wparam, lparam);
	};

	if(params.clientWidth == 0 || params.clientHeight == 0)
	{
		LOG_ERROR("Invalid parameter");
		return nullptr;
	}

	const uint32_t basicStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	const uint32_t extendedStyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;

	const WNDCLASSEXA wnd =
	{
		sizeof(WNDCLASSEXA),                   // UINT        cbSize
		CS_HREDRAW | CS_VREDRAW | CS_OWNDC,    // UINT        style
		wndProc,                               // WNDPROC     lpfnWndProc
		0,                                     // int         cbClsExtra
		0,                                     // int         cbWndExtra
		(HINSTANCE) GetModuleHandleA(nullptr), // HINSTANCE   hInstance
		LoadIconA(nullptr, IDI_APPLICATION),   // HICON       hIcon
		LoadCursorA(nullptr, IDC_ARROW),       // HCURSOR     hCursor
		(HBRUSH) GetStockObject(DKGRAY_BRUSH), // HBRUSH      hbrBackground
		nullptr,                               // LPCWSTR     lpszMenuName
		DF_WINDOW_CLASS_NAME,                  // LPCWSTR     lpszClassName
		nullptr,                               // HICON       hIconSm
	};

	// Register the window class so we can create an instance of it.
	if(!RegisterClassExA(&wnd))
	{
		LOG_ERROR("Failed to register window class '%s'", DF_WINDOW_CLASS_NAME);
		return nullptr;
	}

	// Start with a rect that represents the desired client region size.
	RECT windowRect =
	{
		0,                         // LONG    left
		0,                         // LONG    top
		LONG(params.clientWidth),  // LONG    right
		LONG(params.clientHeight), // LONG    bottom
	};

	switch(params.style)
	{
		case WindowStyle::Standard:
		case WindowStyle::Centered:
			// Adjust the rect so that it represents the dimensions of the entire window.
			AdjustWindowRectEx(&windowRect, basicStyle, false, extendedStyle);
			break;

		default:
			break;
	}

	// Calculate the total size of the window.
	const LONG windowWidth = windowRect.right - windowRect.left;
	const LONG windowHeight = windowRect.bottom - windowRect.top;

	// Create the window.
	HWND hWnd =
		CreateWindowExA(
			extendedStyle,
			wnd.lpszClassName,
			params.windowTitle ? params.windowTitle : "(UNKNOWN)",
			basicStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowWidth,
			windowHeight,
			nullptr,
			nullptr,
			wnd.hInstance,
			nullptr
		);

	if(!hWnd)
	{
		LOG_ERROR("Failed to create window");

		UnregisterClassA(DF_WINDOW_CLASS_NAME, nullptr);
		return nullptr;
	}

	// Get the window's actual position.
	GetWindowRect(hWnd, &windowRect);

	Window* const pOutput = new Window();

	pOutput->m_hWnd = hWnd;
	pOutput->m_pEventListener = pEventListener ? pEventListener : &defaultEventListener;
	pOutput->m_clientWidth = params.clientWidth;
	pOutput->m_clientHeight = params.clientHeight;
	pOutput->m_windowX = int32_t(windowRect.left);
	pOutput->m_windowY = int32_t(windowRect.top);

	// Map the output object to the window handle.
	SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pOutput));

	if(params.style == WindowStyle::Centered || params.style == WindowStyle::Borderless)
	{
		// Find the monitor that most contains the window.
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);

		// Get information about the monitor we're on so we can get at its size.
		GetMonitorInfoA(hMonitor, &monitorInfo);

		const LONG monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		const LONG monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

		switch(params.style)
		{
			case WindowStyle::Centered:
			{
				const LONG halfMonitorWidth = monitorWidth / 2;
				const LONG halfMonitorHeight = monitorHeight / 2;

				const LONG halfWindowWidth = windowWidth / 2;
				const LONG halfWindowHeight = windowHeight / 2;

				// Calculate the new position of the upper-left corner of the window.
				const LONG newPositionX = halfMonitorWidth - halfWindowWidth;
				const LONG newPositionY = halfMonitorHeight - halfWindowHeight;

				// We move the window, but don't change its size.
				MoveWindow(hWnd, newPositionX, newPositionY, windowWidth, windowHeight, false);
				break;
			}

			case WindowStyle::Borderless:
				// Resize the window so it fills the screen.
				SetWindowLongA(hWnd, GWL_STYLE, basicStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(
					hWnd,
					HWND_TOP,
					0, 0,
					monitorWidth,
					monitorHeight,
					SWP_NOACTIVATE | SWP_FRAMECHANGED
				);
				break;

			default:
				// This should never happen.
				assert(false);
				break;
		}
	}

	return pOutput;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Window::Update()
{
	if(m_stateFlags & State::Initialized)
	{
		// Reset the mouse wheel delta before processing window messages.
		m_mouseWheelDelta = 0.0f;

		// Dispatch all enqueued messages received by this window.
		MSG msg;
		while(PeekMessageA(&msg, m_hWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Window::Show()
{
	if((m_stateFlags & State::Initialized) && !(m_stateFlags & State::Visible))
	{
		// Show the window, bring it to the foreground, and focus it for the user.
		ShowWindow(m_hWnd, SW_SHOWNORMAL);
		SetForegroundWindow(m_hWnd);
		SetFocus(m_hWnd);
		UpdateWindow(m_hWnd);

		m_stateFlags |= State::Visible;
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Window::Hide()
{
	if((m_stateFlags & State::Initialized) && (m_stateFlags & State::Visible))
	{
		// Hide the window.
		ShowWindow(m_hWnd, SW_HIDE);

		m_stateFlags &= ~State::Visible;
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Window::SetCursorVisibleState(const bool visible)
{
	if(m_stateFlags & State::Initialized)
	{
		CURSORINFO info = { sizeof(CURSORINFO), };
		GetCursorInfo(&info);

		// Check the current state of the cursor before setting it.
		if((visible && (info.flags & CURSOR_SHOWING))
			|| (!visible && !(info.flags & CURSOR_SHOWING)))
		{
			return;
		}

		ShowCursor((BOOL) visible);
	}
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Window::SetCursorPosition(const int32_t clientX, const int32_t clientY)
{
	if(m_stateFlags & State::Initialized)
	{
		POINT p =
		{
			LONG(clientX), // LONG  x
			LONG(clientY), // LONG  y
		};

		// Convert the input position to absolute screen coordinates.
		ClientToScreen(m_hWnd, &p);
		SetCursorPos(int(p.x), int(p.y));
	}
}

//---------------------------------------------------------------------------------------------------------------------
