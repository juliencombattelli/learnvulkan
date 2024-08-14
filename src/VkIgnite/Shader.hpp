#pragma once

#include "vulkan.hpp"

#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#include <fstream>
#include <stdexcept>

namespace vki {

struct ShaderCompileInfo {
    glslang_stage_t shaderStage;
    const char* inputIdentifier;
    const char* entryPointName = "main";
};

class Shader {
public:
    [[nodiscard]] static vk::UniqueShaderModule compileGlslToSpv(
        vk::Device& device,
        const char* sourceText,
        ShaderCompileInfo shaderCompileInfo)
    {
        const glslang_input_t input = {
            .language = GLSLANG_SOURCE_GLSL,
            .stage = shaderCompileInfo.shaderStage,
            .client = GLSLANG_CLIENT_VULKAN,
            .client_version = GLSLANG_TARGET_VULKAN_1_3,
            .target_language = GLSLANG_TARGET_SPV,
            .target_language_version = GLSLANG_TARGET_SPV_1_5,
            .code = sourceText,
            .default_version = 100,
            .default_profile = GLSLANG_NO_PROFILE,
            .force_default_version_and_profile = false,
            .forward_compatible = false,
            .messages = GLSLANG_MSG_DEFAULT_BIT,
            .resource = glslang_default_resource(),
            .callbacks = {
                .include_system = nullptr,
                .include_local = nullptr,
                .free_include_result = nullptr,
            },
            .callbacks_ctx = nullptr,
        };
        glslang_shader_t* shader = glslang_shader_create(&input);

        if (!glslang_shader_preprocess(shader, &input)) {
            spdlog::error("GLSL preprocessing failed {}", shaderCompileInfo.inputIdentifier);
            spdlog::error("{}", glslang_shader_get_info_log(shader));
            spdlog::error("{}", glslang_shader_get_info_debug_log(shader));
            spdlog::error("{}", input.code);
            glslang_shader_delete(shader);
            throw std::runtime_error("Unable to preprocess shader");
        }

        if (!glslang_shader_parse(shader, &input)) {
            spdlog::error("GLSL parsing failed {}", shaderCompileInfo.inputIdentifier);
            spdlog::error("{}", glslang_shader_get_info_log(shader));
            spdlog::error("{}", glslang_shader_get_info_debug_log(shader));
            spdlog::error("{}", glslang_shader_get_preprocessed_code(shader));
            glslang_shader_delete(shader);
            throw std::runtime_error("Unable to parse shader");
        }

        glslang_program_t* program = glslang_program_create();
        glslang_program_add_shader(program, shader);

        if (!glslang_program_link(
                program,
                GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
            spdlog::error("GLSL linking failed {}", shaderCompileInfo.inputIdentifier);
            spdlog::error("{}", glslang_program_get_info_log(program));
            spdlog::error("{}", glslang_program_get_info_debug_log(program));
            glslang_program_delete(program);
            glslang_shader_delete(shader);
            throw std::runtime_error("Unable to link shader");
        }

        glslang_program_SPIRV_generate(program, shaderCompileInfo.shaderStage);

        size_t size = glslang_program_SPIRV_get_size(program);
        std::vector<uint32_t> shaderBinary;
        shaderBinary.resize(size);
        glslang_program_SPIRV_get(program, shaderBinary.data());

        const char* spirvMessages = glslang_program_SPIRV_get_messages(program);
        if (spirvMessages) {
            spdlog::error("({}) {}", shaderCompileInfo.inputIdentifier, spirvMessages);
        }

        glslang_program_delete(program);
        glslang_shader_delete(shader);

        return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo {
            .flags = {},
            .codeSize = static_cast<size_t>(shaderBinary.size()) * sizeof(uint32_t),
            .pCode = shaderBinary.data(),
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
        if (buffer.back() != '\0') {
            buffer.push_back('\0');
        }

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        return compileGlslToSpv(device, buffer.data(), shaderCompileInfo);
    }
};

} // namespace vki
