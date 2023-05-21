:: Copyright (c) 2023, Zoe J. Bare
::
:: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
:: documentation files (the "Software"), to deal in the Software without restriction, including without limitation
:: the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
:: and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
::
:: The above copyright notice and this permission notice shall be included in all copies or substantial portions
:: of the Software.
::
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
:: TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
:: THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
:: CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
:: IN THE SOFTWARE.

@echo off
pushd %~dp0

set ENV_DIR=_env
set PROJ_DIR=_project
set TMP_DIR=_tmp
set LOCAL_PYTHON=%ENV_DIR%\Scripts\python.exe
set GEN_PROJ_LOG_FILE=%TMP_DIR%\generate-projects.log

:: Verify the local build environment has been setup.
if not exist "%LOCAL_PYTHON%" (
	goto verify_env_failed
)

:: Generate the project files
echo Generating project files ...
"%LOCAL_PYTHON%" make.py --generate-solution=visual-studio-2022 --solution-name=DemoFramework --solution-path=%PROJ_DIR% -o msvc -a x86 -a x64 --at > %GEN_PROJ_LOG_FILE% 2>&1
if not errorlevel 0 goto gen_proj_failed

echo ... success
goto finish

:verify_env_failed
echo ERROR: Failed to find build environment; please run setup.bat
goto exit_with_error

:gen_proj_failed
echo ERROR: Failed to generate project files; see '%GEN_PROJ_LOG_FILE%' for more details
goto exit_with_error

:exit_with_error
popd
exit /b 1

:finish
popd
exit /b 0
