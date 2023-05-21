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

#include <stdarg.h>

//---------------------------------------------------------------------------------------------------------------------

#define LOG_WRITE(msg, ...) DemoFramework::Log::Write(msg, __VA_ARGS__)
#define LOG_ERROR(msg, ...) DemoFramework::Log::Error(__FILE__, __FUNCSIG__, __LINE__, msg, __VA_ARGS__)

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework
{
	class Log;
}

//---------------------------------------------------------------------------------------------------------------------

class DF_API DemoFramework::Log
{
public:

	Log() = delete;

	//! Log a message.
	static void Write(const char* message, ...);

	//! Log an error.
	static void Error(const char* file, const char* function, int line, const char* message, ...);

	//! Open the specified file where log messages will be written to.
	static bool OpenFile(const char* filePath);

	//! Close the log file.
	static void CloseFile();


private:

	static void prv_write(const char*, const char*, int, const char*, va_list, bool);
};

//---------------------------------------------------------------------------------------------------------------------

