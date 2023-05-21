::
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
::

@echo off
pushd %~dp0

set ENV_DIR=_env
set LOG_DIR=_log
set SYS_PYTHON=python.exe
set LOCAL_PYTHON=%ENV_DIR%\Scripts\python.exe
set UPDATE_PYTHON_LOG_FILE=%LOG_DIR%\update-python.log
set INSTALL_CSBUILD_LOG_FILE=%LOG_DIR%\install-csbuild.log

:: Find the Python executable.
where /q %SYS_PYTHON%
if %ERRORLEVEL% NEQ 0 goto find_python_failed

%SYS_PYTHON% --version

:: Remove the existing build environment path.
if exist %ENV_DIR% (
	echo Cleaning build environment ...
	rmdir /q /s %ENV_DIR%
	if %ERRORLEVEL% NEQ 0 goto remove_env_failed
)

if exist %LOG_DIR% (
	echo Cleaning temp directory ...
	rmdir /q /s %LOG_DIR%
	if %ERRORLEVEL% NEQ 0 goto remove_tmp_failed
)

:: Create the setup temp directory.
mkdir %LOG_DIR%
if %ERRORLEVEL% NEQ 0 goto create_tmp_failed

:: Create a new Python virtual environment.
echo Generating build environment ...
%SYS_PYTHON% -m venv _env
if %ERRORLEVEL% NEQ 0 goto create_env_failed

:: Update Python's base packages.
echo Updating local Python packages ...
%LOCAL_PYTHON% -m pip install -U pip wheel setuptools > %UPDATE_PYTHON_LOG_FILE% 2>&1
if %ERRORLEVEL% NEQ 0 goto update_python_pkgs_failed

:: Install csbuild to the local Python virtual environment.
echo Installing csbuild locally ...
%LOCAL_PYTHON% -m pip install -e External/csbuild2 > %INSTALL_CSBUILD_LOG_FILE% 2>&1
if %ERRORLEVEL% NEQ 0 goto install_csbuild_failed

echo ... success
goto finish

:find_python_failed
echo ERROR: Unable to find Python; please make sure it is installed and accessible through the PATH environment variable
goto exit_with_error

:remove_env_failed
echo ERROR: Failed to remove %ENV_DIR% directory
goto exit_with_error

:remove_tmp_failed
echo ERROR: Failed to remove %LOG_DIR% directory
goto exit_with_error

:create_env_failed
echo ERROR: Failed to create %ENV_DIR% directory
goto exit_with_error

:create_tmp_failed
echo ERROR: Failed to create %LOG_DIR% directory
goto exit_with_error

:update_python_pkgs_failed
echo ERROR: Failed to update local Python packages; see '%UPDATE_PYTHON_LOG_FILE%' for more details
goto exit_with_error

:install_csbuild_failed
echo ERROR: Failed to install csbuild; see '%INSTALL_CSBUILD_LOG_FILE%' for more details
goto exit_with_error

:exit_with_error
popd
exit /b 1

:finish
popd
exit /b 0
