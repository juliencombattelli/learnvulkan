# LearnVulkan

Based on https://vulkan-tutorial.com

## Supported platforms

- Linux (GCC, Clang): tested on Ubuntu 22.04 and 24.04
- Windows (MSVC, MinGW): tested on Windows 10 and 11

## Building

### Ubuntu (native)

<details>
<summary>Steps</summary>

- Install the tools required to build the project:
```shell
sudo apt install cmake pkg-config ninja-build build-essential
```

- To build GLFW from source on Linux, a few extra packages must be installed as
  explained in [GLFW documentation](https://www.glfw.org/docs/3.4/compile.html):
```shell
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```

- Download the Vulkan SDK from LunarG. Replace `${SDK_VERSION}` with `latest` or
  any fixed version greater than `1.3.268.0`:
```shell
mkdir VulkanSDK
cd VulkanSDK
wget https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/linux/vulkan_sdk.tar.xz
tar -xvf vulkan_sdk.tar.xz
```

- To build the project run the following commands from project root directory:
```shell
# Use the desired Vulkan SDK
source path/to/VulkanSDK/${SDK_VERSION}/setup-env.sh

# Configure the project with the default system compiler
cmake -S . -B build -G "Ninja Multi-Config"
# Or configure the project with GCC
CC=gcc CXX=g++ cmake -S . -B build -G "Ninja Multi-Config"
# Or configure the project with Clang
CC=clang CXX=clang++ cmake -S . -B build -G "Ninja Multi-Config"

# Build the project
cmake --build build --config release
```

</details>

### Ubuntu (cross-compilation to Windows using MinGW)

<details>
<summary>Steps</summary>

- To cross-compile from Ubuntu to Windows using MinGW, install the following
  packages:
```shell
sudo apt install cmake pkg-config ninja-build mingw-w64 7zip
```

- Then, install the Vulkan SDK for Windows:
```shell
mkdir VulkanSDK-Windows
cd VulkanSDK-Windows
wget https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/windows/vulkan_sdk.exe
7z x vulkan_sdk.exe -o${SDK_VERSION}
```

- To build the project run the following commands from project root directory:
```shell
# Use the desired Vulkan SDK
source path/to/VulkanSDK/${SDK_VERSION}/setup-env.sh

# Configure the project
cmake -S . -B build -G "Ninja Multi-Config" --toolchain cmake/Toolchains/MinGW.cmake

# Build the project
cmake --build build --config release
```

</details>

### Window

<details>
<summary>Steps</summary>

#### Common requirements

- Install the Vulkan SDK for Windows:
```shell
Invoke-WebRequest https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/windows/vulkan_sdk.exe
.\vulkan_sdk.exe
# In the UI select the desired install location and optional components
```

#### MSVC

- Install the Microsoft Visual C/C++ Compiler 2022 at least:
  * if you want the full-fledged Visual Studio IDE, install
    [Visual Studio 2022](https://visualstudio.microsoft.com/vs/)
  * if you prefer just the command line tools, install the
    [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)

- Open a Powershell terminal with x64 build tools available, go at the project
  root location and run the following commands:
```shell
# Ensure the VULKAN_SDK environment variable is setup properly
echo $Env:VULKAN_SDK

# Configure the project
cmake -S . -B build -G "Ninja Multi-Config"

# Build the project
cmake --build build --config release
```

#### MinGW

TODO

</details>

## To do

### High priority

- [-] Stop using FindVulkan as it is always outdated and use CMake config files from SDK
- [-] Use Vulkan 1.3.275.0 as min version to get proper CMake support from SDK
      => CMake config files from SDK are currently broken...
      => Will stay on 1.3.268.0

- [x] Define a custom minimal FindVulkan based on CMake's one
      => Not really minimal, but functionnal
- [x] Define the Vulkan min version once in CMake and propagate it anywhere needed
- [ ] For native builds, require the SDK to be installed
- [ ] For cross compile builds, requires SDK for target system to be installed

### Low priority

- [ ] For cross compile builds, if host tools are required (glslc) build them from sources if not already installed
- [ ] (?) Add support for a VULKAN_SDK_HOST env var for systems having host and target SDK
