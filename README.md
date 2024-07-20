# LearnVulkan

Based on https://vulkan-tutorial.com

## Supported platforms

- Linux (GCC and Clang): tested on Ubuntu 22.04 and 24.04

- Windows (MSVC): tested on Windows 10 and 11

## Install requirements

Download the Vulkan SDK from LunarG. Replace `${SDK_VERSION}` with `latest` or
any fixed version greater than `1.3.264`.

- from Linux:
```shell
mkdir vulkan-sdk
cd vulkan-sdk
wget https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/linux/vulkan_sdk.tar.xz
tar -xvf vulkan_sdk.tar.xz
```

- from Windows (Powershell):
```shell
Invoke-WebRequest https://sdk.lunarg.com/sdk/download/${SDK_VERSION}/windows/vulkan_sdk.exe
.\vulkan_sdk.exe
# In the UI select the desired install location and optional components
```
