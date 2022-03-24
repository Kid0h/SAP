# SAP: Simple Audio Player
A simple cross-platform audio player made in C++

## Usage
```
./sap <file> [volume]
```
example usage:
```bash
./sap "ON TOP.mp3" 25
```

## Dependencies
- [ffmpeg libraries (v4.4 >=)](https://github.com/FFmpeg/FFmpeg) - for audio decoding
- [miniaudio (included)](https://github.com/mackron/miniaudio) - for audio playback
- [fmt (included as a submodule)](https://github.com/fmtlib/fmt) - for easier logging

## Building from source
You'll need to have the ffmpeg libraries already installed on your machine.
if you're using `linux`, CMake will try to automatically detect the ffmpeg libraries and use them.



Please make sure you clone the repository using `--recursive` to clone the needed submodules too.  
```
git clone --recursive https://github.com/Kid0h/SAP
cd SAP
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```
If you're using `Windows` or if CMake can't find the ffmpeg libraries on your machine - you'll need to specifiy an include directory and a link file for each ffmpeg component one by one to CMake, for example:
```bash
cmake -DCOMPONENT_LIBRARIES="C:/ffmpeg/build/component/lib/component.lib" -DCOMPONENT_INCLUDE_DIRS="C:/ffmpeg/build/component/include" ..
```

## Reaching out
If you have any issues or questions - you can open an issue [here](https://github.com/Kid0h/SAP/issues/new)!
