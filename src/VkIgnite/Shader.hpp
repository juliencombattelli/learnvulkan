#pragma once

#include "vulkan.hpp"
#include <shaderc/shaderc.hpp>

#include <fstream>
#include <stdexcept>

namespace vki {

struct ShaderCompileInfo {
    shaderc_shader_kind shaderKind;
    std::string_view inputIdentifier;
    std::string_view entryPointName;
    const shaderc::CompileOptions& option;
};

class Shader {
public:
    [[nodiscard]] static vk::UniqueShaderModule compileGlslToSpv(
        vk::Device& device,
        std::string_view sourceText,
        ShaderCompileInfo shaderCompileInfo)
    {
        shaderc::Compiler compiler;

        shaderc::SpvCompilationResult vertShaderModule = compiler.CompileGlslToSpv(
            sourceText.data(),
            sourceText.size(),
            shaderCompileInfo.shaderKind,
            shaderCompileInfo.inputIdentifier.data(),
            shaderCompileInfo.entryPointName.data(),
            shaderCompileInfo.option);

        if (vertShaderModule.GetCompilationStatus() != shaderc_compilation_status_success) {
            throw std::runtime_error(vertShaderModule.GetErrorMessage());
        }

        std::vector<uint32_t> vertShaderCode(vertShaderModule.cbegin(), vertShaderModule.cend());
        auto vertSize = std::distance(vertShaderCode.begin(), vertShaderCode.end());
        if (vertSize < 0) {
            throw std::runtime_error("Something went really wrong...");
        }

        return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo {
            .flags = {},
            .codeSize = static_cast<size_t>(vertSize) * sizeof(uint32_t),
            .pCode = vertShaderCode.data(),
        });
    }

    [[nodiscard]] static vk::UniqueShaderModule compileGlslToSpvFromFile(
        vk::Device& device,
        std::string filename,
        ShaderCompileInfo shaderCompileInfo)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file) {
            using namespace std::string_literals;
            throw std::runtime_error("Failed to open shader source file "s + filename.data());
        }

        auto fileSize = file.tellg();
        if (fileSize < 0) {
            throw std::runtime_error("Something went really wrong...");
        }
        std::vector<char> buffer(static_cast<size_t>(fileSize));

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return compileGlslToSpv(device, { buffer.begin(), buffer.end() }, shaderCompileInfo);
    }
};

} // namespace vki
