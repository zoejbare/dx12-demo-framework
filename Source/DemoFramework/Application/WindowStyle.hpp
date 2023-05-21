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

namespace DemoFramework
{
	enum class WindowStyle
	{
		//! Standard window style with borders and a title bar.
		Standard,

		/*!
		 * Create the window centered in the middle of the screen.
		 *
		 * Same as the Standard window style, but the position parameters on initialization
		 * are ignored, setting the position to the appropriate position in the center of
		 * the screen where the window is located.
		 */
		Centered,

		/*!
		 * Window style with only a client area, no borders or title bar.
		 *
		 * This will disregard the position and size parameters on initialization
		 * by creating the window to fill the extents of the screen.
		 */
		Borderless,

		//! Total number of window style types.
		Count,
	};
}

//---------------------------------------------------------------------------------------------------------------------
