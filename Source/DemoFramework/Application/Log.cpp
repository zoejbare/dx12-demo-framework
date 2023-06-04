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

#include "Log.hpp"

#include <stdio.h>
#include <time.h>

//---------------------------------------------------------------------------------------------------------------------

namespace DemoFramework { namespace prv {
	FILE* pFile = nullptr;
}}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Log::Write(const char* const message, ...)
{
	if(!message || message[0] == '\0')
	{
		return;
	}

	va_list args;
	va_start(args, message);
	prv_write(nullptr, nullptr, -1, message, args, false);
	va_end(args);
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Log::Error(const char* const file, const char* const function, const int line, const char* const message, ...)
{
	if((file && file[0] == '\0') || (function && function[0] == '\0') || !message || message[0] == '\0')
	{
		return;
	}

	va_list args;
	va_start(args, message);
	prv_write(file, function, line, message, args, true);
	va_end(args);
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Log::prv_write(
	const char* const file,
	const char* const function,
	const int line,
	const char* const message,
	va_list args,
	const bool isError)
{
	assert(message != nullptr && message[0] != '\0');
	assert((file == nullptr) || (file != nullptr && file[0] != '\0'));
	assert((function == nullptr) || (function != nullptr && function[0] != '\0'));

	constexpr const char* const errorTag = "(ERROR) ";
	constexpr size_t maxInt32Length = 11;

	const size_t errorTagLength = strlen(errorTag);

	// Make a copy of the variable args since we need to use it to find the buffer size.
	va_list argsCopy;
	va_copy(argsCopy, args);

	// Determine how long the message buffer needs to be.
	const int msgLength = vsnprintf(nullptr, 0, message, argsCopy);
	va_end(argsCopy);

	if(msgLength > 0)
	{
		char* fileStr = nullptr;
		if(file)
		{
			fileStr = reinterpret_cast<char*>(malloc(strlen(file) + 9));
			sprintf(fileStr, "\n\tFile: %s", file);
		}

		char* funcStr = nullptr;
		if(function)
		{
			funcStr = reinterpret_cast<char*>(malloc(strlen(function) + 9));
			sprintf(funcStr, "\n\tFunc: %s", function);
		}

		char* lineStr = nullptr;
		if(line >= 0)
		{
			lineStr = reinterpret_cast<char*>(malloc(maxInt32Length + 9));
			sprintf(lineStr, "\n\tLine: %" PRId32, line);
		}

		const size_t fileLength = fileStr ? strlen(fileStr) : 0;
		const size_t funcLength = funcStr ? strlen(funcStr) : 0;
		const size_t lineLength = lineStr ? strlen(lineStr) : 0;

		struct timeval
		{
			long tv_sec;
			long tv_usec;
		};

		// Microseconds from 00:00:00 1601-01-01 to 00:00:00 1970-01-01.
		// This is needed since FILETIME reports time since the 1601 date.
		const uint64_t epochOffset = (uint64_t) 116444736000000000ULL;

		SYSTEMTIME sysTime;
		FILETIME fileTime;

		// Get the current time so we can resolve it down to the sub-second.
		GetSystemTime(&sysTime);
		SystemTimeToFileTime(&sysTime, &fileTime);

		ULARGE_INTEGER timeSegment;

		timeSegment.LowPart = fileTime.dwLowDateTime;
		timeSegment.HighPart = fileTime.dwHighDateTime;

		timeval subSecondTime;
		subSecondTime.tv_sec = (long)((timeSegment.QuadPart - epochOffset) / 10000000L);
		subSecondTime.tv_usec = (long)(sysTime.wMilliseconds) * 1000;

		// A timestamp can only be up to a maximum size, so we can statically allocate a buffer for it.
		constexpr const size_t basicTimeStampMaxLength = 80;
		char timeStamp[basicTimeStampMaxLength + 10];
		timeStamp[0] = '\0';

		// Get the current system time.
		const time_t currentTime = time(nullptr);
		const tm* pTimeSpec = localtime(&currentTime);

		// Resolve the current time into a string.
		char temp[basicTimeStampMaxLength];
		strftime(temp, sizeof(temp), "%F, %H:%M:%S", pTimeSpec);

		// Add the millisecond count to make the timestamps more useful.
		snprintf(timeStamp, sizeof(timeStamp), "%s.%03" PRIu16, temp, uint16_t(subSecondTime.tv_usec / 1000));

		// Calculate the length of the fully resolved message string.
		const size_t bufferSize = size_t(msgLength)
			+ strlen(timeStamp)
			+ (isError ? errorTagLength : 0)
			+ fileLength
			+ funcLength
			+ lineLength
			+ 2 // Add two for the timestamp tag brackets.
			+ 1 // Add one for a space after the timestamp.
			+ 1 // Add one for the newline at the end of the message.
			+ 1; // Add one for the null terminator

		char* const buffer = reinterpret_cast<char*>(malloc(bufferSize));
		assert(buffer != nullptr);

		// Copy the timestamp into the message buffer.
		sprintf(buffer, "[%s] ", timeStamp);

		if(isError)
		{
			// Add the error tag to the buffer.
			strcat(buffer, errorTag);
		}

		// Calculate the current length to use as an offset into the buffer.
		size_t currentLength = strlen(buffer);

		// Resolve the message to the end of buffer.
		vsprintf(buffer + currentLength, message, args);

		if(fileStr)
		{
			// Copy the filename into the message buffer.
			strcat(buffer, fileStr);
			free(fileStr);
		}

		if(funcStr)
		{
			// Copy the function name into the message buffer.
			strcat(buffer, funcStr);
			free(funcStr);
		}

		if(lineStr)
		{
			// Copy the line number into the message buffer.
			strcat(buffer, lineStr);
			free(lineStr);
		}

		// Add a newline at the end.
		strcat(buffer, "\n");

		// Write the log message to the Visual Studio debugger.
		OutputDebugStringA(buffer);

		// Determine if we're writing to stdout or stderr.
		FILE* const pOutputStream = (isError ? stderr : stdout);

		// Print the log message to the standard output stream.
		fprintf(pOutputStream, "%s", buffer);
		fflush(pOutputStream);

		if(prv::pFile)
		{
			// Print the log message to the current log file.
			// Flushing a file to disk is significantly more
			// expensive than flushing stdout, but we can avoid
			// doing that since we don't need the output immediately.
			fprintf(prv::pFile, "%s", buffer);
		}

		// Free the message buffer.
		free(buffer);
	}
}

//---------------------------------------------------------------------------------------------------------------------

bool DemoFramework::Log::OpenFile(const char* const filePath)
{
	if(prv::pFile || !filePath || filePath[0] == '\0')
	{
		return false;
	}

	// Attempt to open the log file, truncating any existing contents.
	prv::pFile = fopen(filePath, "w");

	if(!prv::pFile)
	{
		LOG_ERROR("Failed to open log file for writing");
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------------------------------------------------

void DemoFramework::Log::CloseFile()
{
	if(prv::pFile)
	{
		fclose(prv::pFile);
		prv::pFile = nullptr;
	}
}

//---------------------------------------------------------------------------------------------------------------------
