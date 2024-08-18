# ORigin Game Engine

#### Clone the repository recursively `git clone --recursive https://github.com/evangelionmanuhutu/ORigin-Engine.git`

## Build on Windows
#### Please Install `pip install requests`
Required [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) and [Python3](https://www.python.org/downloads/).<br>
Run [gen.bat](gen.bat) to generate Visual Studio .sln file or Makefile.

## Build on Linux tested on Ubuntu-22.04
#### Install 
```bash 
    sudo apt install libwayland-dev libxkbcommon-dev xorg-dev zlib1g-dev libfmt-dev
```
#### Install Mono from [Mono Project](https://www.mono-project.com/download/stable/#download-lin) and execute
```bash
    sudo apt install build-essential mono-complete mono-devel mono-dbg libicu-dev
```
#### Install [Vulkan](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html)
Then run `bash gen.sh` to generate Makefile.
