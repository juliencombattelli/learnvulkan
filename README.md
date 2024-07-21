# LearnVulkan

Based on https://vulkan-tutorial.com

## Supported platforms

- Linux (GCC and Clang): tested on Ubuntu 22.04 and 24.04
- Windows (MSVC): tested on Windows 10 and 11

## Requirements

### Build tools

#### Ubuntu

Install the tools required to build the project:

```shell
sudo apt install cmake pkg-config ninja-build build-essential
```

#### Windows

Install the Microsoft Visual C/C++ Compiler 2022 at least:

- if you want the full-fledged Visual Studio IDE, install
  [Visual Studio 2022](https://visualstudio.microsoft.com/vs/)
- if you prefer just the command line tools, install the
  [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)

### Vulkan SDK

Download the Vulkan SDK from LunarG. Replace `${SDK_VERSION}` with `latest` or
any fixed version greater than `1.3.264`.

#### Linux

```shell
mkdir VulkanSDK
cd VulkanSDK
wget https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/linux/vulkan_sdk.tar.xz
tar -xvf vulkan_sdk.tar.xz
```

#### Windows (from Powershell)

```shell
Invoke-WebRequest https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/windows/vulkan_sdk.exe
.\vulkan_sdk.exe
# In the UI select the desired install location and optional components
```

### GLFW dependencies

#### Ubuntu

To build GLFW on Linux, a few extra packages must be installed as explained in
[GLFW documentation](https://www.glfw.org/docs/3.4/compile.html):

```shell
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```

#### Window

The Visual Studio environment come with all necessary tools to build GLFW, so
nothing specific to do.

## Building

### Ubuntu

In a terminal open at the project root directory, run the following commands:

```shell
# Use the desired Vulkan SDK
source path/to/VulkanSDK/${SDK_VERSION}/setup-env.sh

# Configure the project
cmake -S . -B build -G "Ninja Multi-Config"

# Build the project
cmake --build build --config release
```

### Windows

Open a Powershell terminal with x64 build tools available, go at the project
root location and run the following commands:

```shell
# Ensure the VULKAN_SDK environment variable is setup properly
echo $Env:VULKAN_SDK

# Configure the project
cmake -S . -B build -G "Ninja Multi-Config"

# Build the project
cmake --build build --config release
```
