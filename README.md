# DirectX 12 Demo Framework

A simple framework for building DirectX 12 applications with samples demonstrating its use as well as showcasing graphics research projects.

## License

This project is intended to be free and open source for anyone to use for any reason as long as the included licensing terms are followed. See [LICENSE](LICENSE) for the legal details.

## Build Requirements

The following are the minimum dependencies that must be installed to build this project. Newer versions of the following software should work unless otherwise stated.

- Windows 10, version 1703 (10.0; Build 15063)
- Windows 10 SDK, version 10.0.19041.0
- Visual Studio 2022
- Python 3.6
	- You must be able to access `python.exe` from the system path.

## How To Build

Follow these steps to build the projects. If errors occur during the initial setup, logs with more information can be found in the `_log` directory created within the repo root path.

1. Run `setup.bat`
2. Run `generate-project.bat`
3. Open the solution at `_project/DemoFramework.sln`

## Notes

The `setup.bat` script can fail without returning an error if Python is not installed or not on the system path. The error may look like this:

```
Python was not found; run without arguments to install from the Microsoft Store, or disable this shortcut from Settings > Manage App Execution Aliases.
```

This is because Windows 10 comes with `python.exe` in a special location on the system path that isn't really Python. It's just an app alias that points you to the Microsoft Store if you haven't installed it from there or otherwise.  You can disable this (as mentioned in the above error message) by turning off the app alias, but you will still need to install at least Python 3.6 or later from some source in order to build this project.
