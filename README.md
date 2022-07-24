# PythonLoopback
A Python library to interface with the audio being played on a Windows computer.

Built using CPython extensions on top of some C++ code that interfaces with WASAPI to get the current audio stream and gets some information about that audio stream.

## Requirements
- Windows SDK 10.0.19041.0
- Visual C++ Redistributable for Visual Studio 2015
- Python 3.10 or higher
- NumPy (version unknown)

## Usage
Start by cloning this repository into a local folder. Then you can use pip to install the library like this:
```
pip install --user C:\Users\username\PythonLoopback
```
Make sure the last folder in the path is referring to the folder created by git clone.

## Functions
TODO

## Development
This library was developed in Visual Studio 2019 with the v142 platform toolset. The Windows SDK is 10.0.19041.0, I have no idea if it works with any other SDKs. I have tested using Python 3.10.5 and 3.10.4 (both 64-bit runtime), as well as 3.7.9 (32-bit runtime). It should be compatible with anything above Python 3.7.9, but I haven't tested that yet.

## Example program
This program simply prints out the current amplitude:
```python
from PythonLoopback import get_current_amplitude

while True:
    amp = get_current_amplitude()
    print(amp)
```

This program records a 1 second buffer and replays it after 5 seconds:
```python
from PythonLoopback import record_buffer
import numpy as np
import simpleaudio as sa
import time

buffer = record_buffer(1.0)
print("Recorded buffer, playing back in 5s")
time.sleep(5)
play_obj = sa.play_buffer(buffer, 2, 2, 48000)
play_obj.wait_done()
```
