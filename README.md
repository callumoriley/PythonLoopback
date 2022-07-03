# PythonLoopback
A Python library to interface with the audio being played on a Windows computer

## Requirements
Windows SDK 10.0.19041.0\
Python 3

## Usage
Start by cloning this repository into a local folder. Then you can use pip to install the library like this:
```
pip install --user C:\Users\username\PythonLoopback
```
Make sure the last folder in the path is referring to the folder created by git clone.

## Development
This library was developed in Visual Studio 2019 with the v142 platform toolset. The Windows SDK is 10.0.19041.0, I have no idea if it works with any other SDKs. The compilation target is x86, even though I'm using an x64 computer. I have only tested it with Python 3.7.9 with the 32-bit runtime.
